<#
总流程与五个功能模块

upload.ps1 是唯一入口，负责安排执行顺序、选择待上传仓库和汇总结果。
upload 目录中的五个模块负责具体功能：

1. Network.psm1：网络检查与连接准备
   总入口调用 Initialize-UploadNetwork，读取系统代理、检查公网出口，
   返回 Proxy 和 SshCommand。两个仓库共用这份连接参数，只检查一次。
   网络检查失败立即退出，不继续检查仓库或执行上传。

2. Git.psm1：两个仓库共用的 Git 操作
   总入口调用 Get-UploadRepositoryState，校验远程地址、main 分支和冲突状态，
   检查新增、修改、删除、未推送提交，以及基础库的私有目录隔离规则。
   这一阶段读取本地状态和远程缓存，形成待上传列表。
   后续由 Basic / Private 调用 Prepare-UploadRepository：通过 SSH fetch，
   确认远程没有领先或分叉，再暂存、提交；Push-UploadRepository 负责实际推送。

3. Basic.psm1：基础库的上传流程
   总入口判断基础库有变化，或存在待补发的博客通知时，调用 Invoke-BasicUpload。
   它先调用 Git 模块准备提交并推送；只有配置本机令牌时才保存额外通知记录。
   原 trigger-blog.yml 保持由 GitHub 执行；本地令牌存在时才额外调用 Blog 模块。
   如果只是补发通知，不创建空提交，也不重复推送已经上传的提交。

4. Blog.psm1：可选的本地博客通知与重试记录
   由 Basic.psm1 调用，总入口不重复发送通知。
   没有本机 BLOG_TRIGGER_TOKEN 时直接跳过，不报错，不影响原 GitHub 工作流。
   配置本机令牌后，通过同一代理向 evil0knight/quartz
   发送 notes-updated 事件；收到 GitHub 的成功响应后清除重试记录。
   通知失败保留 .git/blog-notification.pending，下次运行总入口时再次处理。
   通知成功表示 GitHub 接收了事件，实际博客构建和发布仍由 Quartz 执行。

5. Private.psm1：私有库的上传流程
   总入口判断 private-notes 有变化时，调用 Invoke-PrivateUpload。
   它调用 Git 模块准备提交并通过 SSH 推送，不调用 Blog 模块，
   不读取博客令牌，也不复制基础库原目录中的私有笔记。

实际调用顺序：
  总入口 → Network：检查网络并取得连接参数
         → Git：分别检查基础库、私有库，选出待处理项
         → 基础库待处理：Basic → Git 准备提交 → Git 推送 → GitHub 原工作流
                         有本机令牌时：推送前 Blog 保存记录，成功后 Blog 额外通知
         → 私有库待处理：Private → Git 准备提交 → Git 推送
         → 汇总退出码

补充规则：
  - 正常运行先检查网络，即使最后发现两个仓库均无变化，也会完成网络检查。
  - -DryRun 跳过网络与上传，只检查本地状态并显示待处理项。
  - 只有基础库变化就只执行 Basic，只有私有库变化就只执行 Private。
  - 两个库都需要处理时，先基础库后私有库；一个失败仍继续另一个。
  - 推送失败保留本地提交；博客通知失败保留通知记录，重跑总入口即可重试。
  - 退出码 0：成功或无更新；退出码 1：任一检查、上传或通知步骤失败。
#>
# 运行：powershell -NoProfile -ExecutionPolicy Bypass -File .\.github\upload.ps1
# 可选：-DryRun 只预览；-Message "更新学习笔记" 自定义提交说明。
[CmdletBinding()]
param(
    [string]$Message = ('同步笔记 {0}' -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss')),
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'upload/Network.psm1') -DisableNameChecking
Import-Module (Join-Path $PSScriptRoot 'upload/Git.psm1') -DisableNameChecking
Import-Module (Join-Path $PSScriptRoot 'upload/Basic.psm1') -DisableNameChecking
Import-Module (Join-Path $PSScriptRoot 'upload/Private.psm1') -DisableNameChecking

if ([string]::IsNullOrWhiteSpace($Message)) { Write-Host '提交说明不能为空。'; exit 1 }

# 第一步 / Network.psm1：检查代理出口并准备 SSH 连接参数，两个仓库共用本次检查结果。
# DryRun 只做本地预览，因此跳过网络检查和后面的上传。
$transport = $null
if (-not $DryRun) {
    try { $transport = Initialize-UploadNetwork }
    catch { Write-Host "网络检查失败：$($_.Exception.Message)"; exit 1 }
}

# 第二步 / Git.psm1：分别判断基础库和私有库是否有变化，形成待上传列表。
# 根据脚本位置定位工作区，允许从任意目录调用。
$workspaceRoot = Split-Path $PSScriptRoot -Parent
$repositories = @(
    [pscustomobject]@{ Root = $workspaceRoot; Url = 'git@github.com:evil0knight/Basic_learning_record.git'; Public = $true },
    [pscustomobject]@{ Root = (Join-Path $workspaceRoot 'private-notes'); Url = 'git@github.com:evil0knight/private-notes.git'; Public = $false }
)
$pending = @()
$failed = $false
foreach ($repository in $repositories) {
    $name = Split-Path $repository.Root -Leaf
    try {
        $state = Get-UploadRepositoryState $repository.Root $repository.Url $repository.Public
        $blogPending = $repository.Public -and (Test-BasicBlogPending $repository.Root)
        if ($state.Pending -or $blogPending) {
            Write-Host "[$name] 待处理；本地缓存显示未推送提交：$($state.Ahead)"
            if (-not $state.RemoteKnown) { Write-Host '尚无远程 main 缓存，需要联网核实。' }
            if ($state.Status) { Write-Host $state.Status }
            if ($blogPending) { Write-Host '存在待补发的公开博客更新通知。' }
            $pending += $state
        }
        else { Write-Host "[$name] 无本地更新，跳过。" }
    }
    catch { Write-Host "[$name] 检查失败：$($_.Exception.Message)"; $failed = $true }
}
if ($DryRun) { Write-Host '预览结束：未联网、暂存、提交或推送。'; exit ([int]$failed) }

# 第三步 / Basic.psm1、Private.psm1：按待上传列表调用对应模块。
# Basic 调用 Git 上传，有本机令牌时才额外调用 Blog；Private 只调用 Git 上传。
foreach ($state in $pending) {
    $name = Split-Path $state.Root -Leaf
    try {
        if ($state.Public) { Invoke-BasicUpload $state $Message $transport }
        else { Invoke-PrivateUpload $state $Message $transport }
    }
    catch {
        # 一个仓库失败仍继续另一个；保留本地提交和通知记录供下次重试。
        Write-Host "[$name] 上传失败：$($_.Exception.Message)"
        $failed = $true
    }
}

# 第四步：以退出码汇总本次结果，0 为成功或无更新，1 为存在失败。
exit ([int]$failed)