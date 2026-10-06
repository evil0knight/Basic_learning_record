# 博客通知：读取令牌、调用 Quartz API、保存和清除重试记录；不执行 Git。

function Get-BlogTriggerToken {
    # 本地通知是可选功能；GitHub Actions 使用自己保存的 Secret，不依赖这里。
    $token = $env:BLOG_TRIGGER_TOKEN
    if ([string]::IsNullOrWhiteSpace($token)) { $token = [Environment]::GetEnvironmentVariable('BLOG_TRIGGER_TOKEN', 'User') }
    return $token
}

function Test-BlogNotificationEnabled {
    return -not [string]::IsNullOrWhiteSpace((Get-BlogTriggerToken))
}

function Get-BlogPendingCommit {
    param([string]$PendingPath)
    if (-not (Test-Path -LiteralPath $PendingPath)) { return $null }
    $sha = [IO.File]::ReadAllText($PendingPath).Trim()
    if ($sha -notmatch '^[a-f0-9]{40,64}$') { throw '博客通知重试记录无效，请检查 .git/blog-notification.pending。' }
    return $sha
}

function Set-BlogPendingCommit {
    param([string]$PendingPath, [string]$Commit)
    # 调用方传入文件路径，模块无需了解 Git 目录结构；文件中只保存提交号。
    if ($Commit -notmatch '^[a-f0-9]{40,64}$') { throw '待通知提交号无效。' }
    [IO.File]::WriteAllText($PendingPath, $Commit, (New-Object Text.UTF8Encoding $false))
}

function Send-BlogNotification {
    param([string]$PendingPath, [string]$Proxy)
    # 没有本机令牌直接跳过，不读取重试记录、不联网，也不影响 GitHub 原工作流。
    $token = Get-BlogTriggerToken
    if ([string]::IsNullOrWhiteSpace($token)) {
        return
    }
    # 编排层负责确认提交已推送，本模块只负责通知和成功后的记录清理。
    if (-not (Get-BlogPendingCommit $PendingPath)) { return }
    $previousTls = [Net.ServicePointManager]::SecurityProtocol
    try {
        [Net.ServicePointManager]::SecurityProtocol = $previousTls -bor [Net.SecurityProtocolType]::Tls12
        # SSH 负责上传，HTTPS API 负责通知；二者使用同一已检查的代理。
        # 不跟随重定向，不输出请求头或原始异常，避免令牌出现在日志中。
        $response = Invoke-WebRequest -UseBasicParsing -Method Post `
            -Uri 'https://api.github.com/repos/evil0knight/quartz/dispatches' `
            -Proxy "http://$Proxy" -TimeoutSec 20 -MaximumRedirection 0 `
            -Headers @{ Authorization = "Bearer $token"; Accept = 'application/vnd.github+json'; 'X-GitHub-Api-Version' = '2022-11-28'; 'User-Agent' = 'Basic-learning-record-uploader' } `
            -ContentType 'application/json' -Body '{"event_type":"notes-updated"}' -ErrorAction Stop
        if ([int]$response.StatusCode -ne 204) { throw 'Unexpected API response' }
    }
    catch { throw '笔记已上传，博客通知失败；请检查网络及 BLOG_TRIGGER_TOKEN 的 Quartz 仓库 Contents 写权限。再次运行会补发。' }
    finally { [Net.ServicePointManager]::SecurityProtocol = $previousTls }
    # 只有 GitHub 确认接收事件后才清除记录；超时重试可能重复触发一次构建。
    Remove-Item -LiteralPath $PendingPath -ErrorAction Stop
    Write-Host '[公开博客] 更新通知已发送；部署结果请查看 evil0knight/quartz 的 Actions。'
}

Export-ModuleMember -Function Test-BlogNotificationEnabled, Get-BlogPendingCommit, Set-BlogPendingCommit, Send-BlogNotification
