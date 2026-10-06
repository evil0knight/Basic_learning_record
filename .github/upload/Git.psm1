# Git 操作：命令执行、仓库校验、暂存提交和推送；不处理代理探测或博客通知。

function Invoke-UploadGit {
    param([string]$Root, [string[]]$Arguments, [string]$SshCommand)

    # Windows PowerShell 不会因为原生命令返回非零而自动抛错，必须检查退出码。
    # 临时允许 stderr 输出，避免 Git 正常的进度信息被 PowerShell 当作终止错误。
    $previousPreference = $ErrorActionPreference
    $previousSshCommand = $env:GIT_SSH_COMMAND
    $previousSshVariant = $env:GIT_SSH_VARIANT
    $ErrorActionPreference = 'Continue'
    try {
        $options = @('--no-optional-locks', '-C', $Root, '-c', 'core.quotepath=false')
        if ($SshCommand) {
            # 环境变量优先于 core.sshCommand，临时覆盖才能保证不会被旧配置绕过代理。
            $env:GIT_SSH_COMMAND = $SshCommand
            $env:GIT_SSH_VARIANT = 'ssh'
        }
        $output = @(& git @options @Arguments 2>&1)
        $code = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $previousPreference
        $env:GIT_SSH_COMMAND = $previousSshCommand
        $env:GIT_SSH_VARIANT = $previousSshVariant
    }
    [pscustomobject]@{ Code = $code; Text = (($output | ForEach-Object { "$_" }) -join "`n") }
}

function Invoke-UploadGitChecked {
    param([string]$Root, [string[]]$Arguments, [string]$SshCommand)
    $result = Invoke-UploadGit $Root $Arguments $SshCommand
    if ($result.Code -ne 0) { throw "git $($Arguments[0]) 失败：$($result.Text)" }
    return $result.Text
}

function Assert-PublicPrivacy {
    param([string]$Root)
    # 同时检查已跟踪文件和忽略规则，防止私有仓库或原始私有目录进入公开提交。
    foreach ($path in @('private-notes', '术中自有万钟粟/求职', '术中自有万钟粟/日常')) {
        $tracked = Invoke-UploadGitChecked $Root @('ls-files', '--', $path)
        if ($tracked) { throw "公开仓库已跟踪私有路径 $path，请先处理，脚本不会上传该仓库。" }
        $ignored = Invoke-UploadGit $Root @('check-ignore', '--no-index', '-q', '--', "$path/__upload_privacy_probe__")
        if ($ignored.Code -ne 0) { throw "公开仓库缺少私有路径 $path 的忽略规则。" }
    }
}

function Get-UploadRepositoryState {
    param([string]$Root, [string]$ExpectedUrl, [bool]$Public)

    if (-not (Test-Path -LiteralPath (Join-Path $Root '.git'))) { throw "未找到 Git 仓库：$Root" }
    # 显式校验 fetch/push 地址，避免 origin 被改动后误传到其他仓库。
    foreach ($arguments in @(@('remote', 'get-url', '--all', 'origin'), @('remote', 'get-url', '--push', '--all', 'origin'))) {
        $url = Invoke-UploadGitChecked $Root $arguments
        if ($url -cne $ExpectedUrl) { throw "origin 地址不符合预期，应为 $ExpectedUrl" }
    }
    $branch = Invoke-UploadGitChecked $Root @('branch', '--show-current')
    if ($branch -ne 'main') { throw '仅允许上传 main 分支；请先完成分支切换。' }
    foreach ($marker in @('MERGE_HEAD', 'CHERRY_PICK_HEAD', 'REVERT_HEAD', 'rebase-merge', 'rebase-apply', 'sequencer', 'BISECT_START')) {
        $markerPath = Invoke-UploadGitChecked $Root @('rev-parse', '--git-path', $marker)
        if (-not [IO.Path]::IsPathRooted($markerPath)) { $markerPath = Join-Path $Root $markerPath }
        if (Test-Path -LiteralPath $markerPath) { throw "仓库存在未完成的 Git 操作：$marker" }
    }
    if (Invoke-UploadGitChecked $Root @('ls-files', '--unmerged')) { throw '仓库存在未解决的冲突。' }
    if ($Public) { Assert-PublicPrivacy $Root }
    $status = Invoke-UploadGitChecked $Root @('status', '--porcelain=v1', '--untracked-files=all')
    $head = Invoke-UploadGitChecked $Root @('rev-parse', '--verify', 'HEAD')
    $remote = Invoke-UploadGit $Root @('rev-parse', '--verify', 'refs/remotes/origin/main')
    # 这里比较的是本地缓存；联网 fetch 后还会重新判断领先/落后关系。
    $ahead = 0
    if ($remote.Code -eq 0) {
        $ahead = [int](Invoke-UploadGitChecked $Root @('rev-list', '--count', 'refs/remotes/origin/main..HEAD'))
    }
    [pscustomobject]@{
        Root = $Root; Url = $ExpectedUrl; Public = $Public; Status = $status
        Pending = ([bool]$status -or $ahead -gt 0 -or $remote.Code -ne 0)
        Ahead = $ahead; RemoteKnown = ($remote.Code -eq 0); Head = $head
    }
}

function Prepare-UploadRepository {
    param($State, [string]$CommitMessage, [string]$SshCommand)
    $root = $State.Root
    # fetch 同时完成 SSH 认证/连通性检查；失败时尚未暂存或提交笔记。
    $null = Invoke-UploadGitChecked $root @('fetch', '--no-tags', 'origin', '+refs/heads/main:refs/remotes/origin/main') $SshCommand
    $behind = [int](Invoke-UploadGitChecked $root @('rev-list', '--count', 'HEAD..refs/remotes/origin/main'))
    if ($behind -gt 0) { throw '远程 main 领先或已分叉，请手动同步并处理后重试；脚本不会自动合并或强推。' }
    # 联网期间用户可能又编辑文件，因此暂存前重新检查仓库状态和隐私隔离。
    $current = Get-UploadRepositoryState $root $State.Url $State.Public
    if ($current.Status) {
        # 不把忽略目录列入 pathspec；由 .gitignore 排除，并在暂存后再次验证。
        $null = Invoke-UploadGitChecked $root @('add', '-A')
        if ($State.Public) { Assert-PublicPrivacy $root }
        $diff = Invoke-UploadGit $root @('diff', '--cached', '--quiet')
        if ($diff.Code -eq 1) {
            $null = Invoke-UploadGitChecked $root @('commit', '-m', $CommitMessage)
        }
        elseif ($diff.Code -ne 0) { throw "检查暂存区失败：$($diff.Text)" }
    }
    $ahead = [int](Invoke-UploadGitChecked $root @('rev-list', '--count', 'refs/remotes/origin/main..HEAD'))
    # 返回结构化结果，由编排层决定后续动作；Git 模块不关心博客是否需要更新。
    return [pscustomobject]@{
        HasCommits = ($ahead -gt 0)
        Head = (Invoke-UploadGitChecked $root @('rev-parse', 'HEAD'))
    }
}

function Push-UploadRepository {
    param([string]$Root, [string]$SshCommand)
    # 显式 refspec 避免受 push.default 影响；关闭自动跟推标签及子模块。
    $null = Invoke-UploadGitChecked $Root @('-c', 'push.followTags=false', 'push', '--recurse-submodules=no', 'origin', 'HEAD:refs/heads/main') $SshCommand
}

Export-ModuleMember -Function Invoke-UploadGit, Invoke-UploadGitChecked, Get-UploadRepositoryState, Prepare-UploadRepository, Push-UploadRepository
