# 网络准备：读取系统代理、检查出口、构造 SSH 连接参数；不操作仓库。

function Get-UploadProxy {
    $settings = Get-ItemProperty 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Internet Settings' -ErrorAction Stop
    if ($settings.ProxyEnable -ne 1 -or -not $settings.ProxyServer) { throw '未开启 Windows 系统代理，请先开启梯子的系统代理。' }
    $server = [string]$settings.ProxyServer
    if ($server.Contains('=')) {
        $entries = @{}
        foreach ($item in $server.Split(';')) {
            $parts = $item.Split('=', 2)
            if ($parts.Count -eq 2) { $entries[$parts[0].Trim().ToLowerInvariant()] = $parts[1].Trim() }
        }
        $server = $entries['https']
        if (-not $server) { $server = $entries['http'] }
    }
    # 只支持 HTTP CONNECT 代理。限制字符也防止地址被解释成 SSH shell 指令。
    if ($server -notmatch '^(?:http://)?(?<host>[a-zA-Z0-9.-]+):(?<port>\d{1,5})/?$') {
        throw '系统代理格式不支持，需要 host:port 格式的 HTTP 代理（支持 Clash mixed 端口）。'
    }
    $proxyPort = [int]$Matches.port
    if ($proxyPort -lt 1 -or $proxyPort -gt 65535) { throw '系统代理端口无效。' }
    return ('{0}:{1}' -f $Matches.host, $proxyPort)
}

function Assert-UploadExitAddress {
    param([string]$Proxy)
    # 显式指定代理，不能用直连 IP 来代表 SSH 即将使用的出口。
    $previousTls = [Net.ServicePointManager]::SecurityProtocol
    try {
        [Net.ServicePointManager]::SecurityProtocol = $previousTls -bor [Net.SecurityProtocolType]::Tls12
        $info = Invoke-RestMethod -Uri 'https://api.country.is/' -Proxy "http://$Proxy" -TimeoutSec 15 -ErrorAction Stop
    }
    finally { [Net.ServicePointManager]::SecurityProtocol = $previousTls }
    $parsedIp = $null
    $country = [string]$info.country
    if (-not [Net.IPAddress]::TryParse([string]$info.ip, [ref]$parsedIp) -or
        $country -cnotmatch '^[A-Z]{2}$' -or $country -in @('XX', 'ZZ')) {
        throw '公网出口查询结果不明确，已停止上传。'
    }
    Write-Host "代理：$Proxy；公网 IP：$($info.ip)；国家/地区：$country"
    if ($country -eq 'CN') { throw '当前出口位于中国大陆，按配置停止上传。' }
}

function ConvertTo-UploadShellLiteral {
    param([string]$Value)
    # core.sshCommand 由 Git 的 shell 解析，路径中的空格和单引号都需要正确转义。
    return "'" + $Value.Replace("'", "'\''") + "'"
}

function Initialize-UploadNetwork {
    $proxy = Get-UploadProxy
    Assert-UploadExitAddress $proxy
    $gitExe = (Get-Command git -CommandType Application -ErrorAction Stop).Source
    $gitRoot = Split-Path (Split-Path $gitExe -Parent) -Parent
    $connect = Join-Path $gitRoot 'mingw64/bin/connect.exe'
    $ssh = Join-Path $gitRoot 'usr/bin/ssh.exe'
    if (-not (Test-Path -LiteralPath $connect) -or -not (Test-Path -LiteralPath $ssh)) {
        throw '未找到 Git for Windows 自带的 connect.exe 或 ssh.exe，请检查 Git 安装。'
    }
    $proxyCommand = 'ProxyCommand="{0}" -H {1} %h %p' -f $connect.Replace('\', '/'), $proxy
    # 使用 Git 自带 SSH；忽略其他 SSH 配置，防止代理或主机选项覆盖本次出口。
    # 不自动接受陌生主机密钥；首次使用需自行核对 GitHub 指纹并加入 known_hosts。
    $sshCommand = ((ConvertTo-UploadShellLiteral $ssh.Replace('\', '/')) +
        ' -F /dev/null -o BatchMode=yes -o StrictHostKeyChecking=yes -o ConnectTimeout=15' +
        ' -o ServerAliveInterval=15 -o ServerAliveCountMax=2 -o Hostname=ssh.github.com -p 443 -o ' +
        (ConvertTo-UploadShellLiteral $proxyCommand))
    return [pscustomobject]@{ SshCommand = $sshCommand; Proxy = $proxy }
}

Export-ModuleMember -Function Initialize-UploadNetwork
