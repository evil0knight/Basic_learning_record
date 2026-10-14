# 私有库上传：提交、推送，并通知 Quartz 重新构建私有笔记。
Import-Module (Join-Path $PSScriptRoot 'Git.psm1') -DisableNameChecking

function Get-PrivateBlogTriggerToken {
    $token = $env:PRIVATE_BLOG_TRIGGER_TOKEN
    if ([string]::IsNullOrWhiteSpace($token)) { $token = $env:BLOG_TRIGGER_TOKEN }
    if ([string]::IsNullOrWhiteSpace($token)) { $token = [Environment]::GetEnvironmentVariable('PRIVATE_BLOG_TRIGGER_TOKEN', 'User') }
    if ([string]::IsNullOrWhiteSpace($token)) { $token = [Environment]::GetEnvironmentVariable('BLOG_TRIGGER_TOKEN', 'User') }
    return $token
}

function Get-PrivateBlogPendingPath {
    param([string]$Root)
    $path = Invoke-UploadGitChecked $Root @('rev-parse', '--git-path', 'private-blog-notification.pending')
    if (-not [IO.Path]::IsPathRooted($path)) { $path = Join-Path $Root $path }
    return $path
}

function Test-PrivateBlogPending {
    param([string]$Root)
    return Test-Path -LiteralPath (Get-PrivateBlogPendingPath $Root)
}

function Set-PrivateBlogPending {
    param([string]$Path, [string]$Commit)
    if ($Commit -notmatch '^[a-f0-9]{40,64}$') { throw '私有博客待通知提交号无效。' }
    [IO.File]::WriteAllText($Path, $Commit, (New-Object Text.UTF8Encoding $false))
}

function Send-PrivateBlogNotification {
    param([string]$PendingPath, [string]$Proxy)
    $token = Get-PrivateBlogTriggerToken
    if ([string]::IsNullOrWhiteSpace($token)) {
        Write-Host '[私有博客] 未配置 PRIVATE_BLOG_TRIGGER_TOKEN（或 BLOG_TRIGGER_TOKEN），已保留通知记录。'
        return
    }
    $commit = [IO.File]::ReadAllText($PendingPath).Trim()
    if ($commit -notmatch '^[a-f0-9]{40,64}$') { throw '私有博客通知重试记录无效。' }
    $repository = $env:PRIVATE_BLOG_REPOSITORY
    if ([string]::IsNullOrWhiteSpace($repository)) { $repository = 'evil0knight/quartz' }
    $eventType = $env:PRIVATE_BLOG_EVENT_TYPE
    if ([string]::IsNullOrWhiteSpace($eventType)) { $eventType = 'notes-updated' }
    $body = @{ event_type = $eventType; client_payload = @{ source = 'private-notes'; commit = $commit } } | ConvertTo-Json -Compress
    $previousTls = [Net.ServicePointManager]::SecurityProtocol
    try {
        [Net.ServicePointManager]::SecurityProtocol = $previousTls -bor [Net.SecurityProtocolType]::Tls12
        $params = @{
            UseBasicParsing = $true; Method = 'Post'; Uri = "https://api.github.com/repos/$repository/dispatches"
            TimeoutSec = 20; MaximumRedirection = 0
            Headers = @{ Authorization = "Bearer $token"; Accept = 'application/vnd.github+json'; 'X-GitHub-Api-Version' = '2022-11-28'; 'User-Agent' = 'Basic-learning-record-uploader' }
            ContentType = 'application/json'; Body = $body; ErrorAction = 'Stop'
        }
        if (-not [string]::IsNullOrWhiteSpace($Proxy)) { $params.Proxy = "http://$Proxy" }
        $response = Invoke-WebRequest @params
        if ([int]$response.StatusCode -ne 204) { throw 'Unexpected API response' }
        Remove-Item -LiteralPath $PendingPath -ErrorAction Stop
        Write-Host '[私有博客] Quartz 更新通知已发送。'
    }
    catch { throw '私有笔记已上传，但 Quartz 通知失败；请检查 PRIVATE_BLOG_TRIGGER_TOKEN、仓库 Actions 和网络。再次运行会补发。' }
    finally { [Net.ServicePointManager]::SecurityProtocol = $previousTls }
}

function Invoke-CloudflarePagesDeployHook {
    param([string]$Proxy)
    $hook = $env:CLOUDFLARE_PAGES_DEPLOY_HOOK
    if ([string]::IsNullOrWhiteSpace($hook) -or $hook -eq '你的 Deploy Hook URL') { return $false }
    if ($hook -notmatch '^https://') { throw 'CLOUDFLARE_PAGES_DEPLOY_HOOK 必须是 https:// 开头的 Deploy Hook URL。' }
    try {
        $params = @{ UseBasicParsing = $true; Method = 'Post'; Uri = $hook; TimeoutSec = 30; ErrorAction = 'Stop' }
        if (-not [string]::IsNullOrWhiteSpace($Proxy)) { $params.Proxy = "http://$Proxy" }
        $response = Invoke-WebRequest @params
        if ([int]$response.StatusCode -lt 200 -or [int]$response.StatusCode -ge 300) { throw 'Unexpected API response' }
        Write-Host '[私有博客] Cloudflare Pages 部署已触发。'
        return $true
    }
    catch { throw '私有笔记已上传，但 Cloudflare Pages 触发失败；请检查 Deploy Hook URL 和网络。' }
}
function Invoke-PrivateUpload {
    param($State, [string]$CommitMessage, $Transport)
    $ErrorActionPreference = 'Stop'
    $pendingPath = Get-PrivateBlogPendingPath $State.Root
    $prepared = Prepare-UploadRepository $State $CommitMessage $Transport.SshCommand
    if ($prepared.HasCommits) {
        Set-PrivateBlogPending $pendingPath $prepared.Head
        Push-UploadRepository $State.Root $Transport.SshCommand
        Write-Host '[私有库] 推送成功。'
    }
    else { Write-Host '[私有库] 没有需要推送的提交。' }
    if (Test-Path -LiteralPath $pendingPath) {
        $commit = [IO.File]::ReadAllText($pendingPath).Trim()
        $included = Invoke-UploadGit $State.Root @('merge-base', '--is-ancestor', $commit, 'refs/remotes/origin/main')
        if ($included.Code -ne 0) { throw '私有笔记提交尚未进入远程 main，保留通知记录等待下次上传。' }
        Send-PrivateBlogNotification $pendingPath $Transport.Proxy`r`n        $null = Invoke-CloudflarePagesDeployHook $Transport.Proxy
    }
}

Export-ModuleMember -Function Test-PrivateBlogPending, Invoke-PrivateUpload
