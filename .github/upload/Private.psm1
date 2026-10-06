# 私有库上传：只提交和推送，不复制原目录，也不依赖博客模块或令牌。
Import-Module (Join-Path $PSScriptRoot 'Git.psm1') -DisableNameChecking

function Invoke-PrivateUpload {
    param($State, [string]$CommitMessage, $Transport)
    $ErrorActionPreference = 'Stop'
    $prepared = Prepare-UploadRepository $State $CommitMessage $Transport.SshCommand
    if ($prepared.HasCommits) {
        Push-UploadRepository $State.Root $Transport.SshCommand
        Write-Host '[私有库] 推送成功。'
    }
    else { Write-Host '[私有库] 没有需要推送的提交。' }
}

Export-ModuleMember -Function Invoke-PrivateUpload
