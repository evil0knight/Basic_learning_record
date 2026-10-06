# 基础库上传：提交、推送及公开博客通知；不操作私有库。
Import-Module (Join-Path $PSScriptRoot 'Git.psm1') -DisableNameChecking
Import-Module (Join-Path $PSScriptRoot 'Blog.psm1') -DisableNameChecking

function Get-BasicBlogPendingPath {
    param([string]$Root)
    $path = Invoke-UploadGitChecked $Root @('rev-parse', '--git-path', 'blog-notification.pending')
    if (-not [IO.Path]::IsPathRooted($path)) { $path = Join-Path $Root $path }
    return $path
}

function Test-BasicBlogPending {
    param([string]$Root)
    # 未配置本机令牌时，本地通知不参与待上传判断，原 GitHub 工作流照常工作。
    if (-not (Test-BlogNotificationEnabled)) { return $false }
    return (Test-Path -LiteralPath (Get-BasicBlogPendingPath $Root))
}

function Invoke-BasicUpload {
    param($State, [string]$CommitMessage, $Transport)
    $ErrorActionPreference = 'Stop'
    $root = $State.Root
    $prepared = Prepare-UploadRepository $State $CommitMessage $Transport.SshCommand
    $notifyLocally = Test-BlogNotificationEnabled
    $pendingPath = $null
    if ($notifyLocally) { $pendingPath = Get-BasicBlogPendingPath $root }
    if ($prepared.HasCommits) {
        # 推送前保存通知任务，进程中断或通知失败时可在下次运行补发。
        if ($notifyLocally) { Set-BlogPendingCommit $pendingPath $prepared.Head }
        Push-UploadRepository $root $Transport.SshCommand
        Write-Host '[基础库] 推送成功。'
    }
    else { Write-Host '[基础库] 没有需要推送的提交。' }

    if (-not $notifyLocally) {
        Write-Host '[公开博客] 未配置本机令牌，跳过本地通知；由 GitHub 原工作流处理。'
        return
    }
    $commit = Get-BlogPendingCommit $pendingPath
    if ($commit) {
        # 确认提交确实进入远程 main 后再调用 API，不通知尚未上传的内容。
        $included = Invoke-UploadGit $root @('merge-base', '--is-ancestor', $commit, 'refs/remotes/origin/main')
        if ($included.Code -ne 0) { throw '待通知提交尚未进入远程 main，保留记录等待下次上传。' }
        Send-BlogNotification $pendingPath $Transport.Proxy
    }
}

Export-ModuleMember -Function Test-BasicBlogPending, Invoke-BasicUpload
