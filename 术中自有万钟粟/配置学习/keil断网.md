# keil断网

[←MOC](MOC.md) | [←主页](../../index.md)

> 网上的[防火墙方法](https://blog.csdn.net/weixin_65672107/article/details/146602444?ops_request_misc=elastic_search_misc&request_id=94e6a7b409b6baa33dfa2391a317eb78&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~sobaiduend~default-4-146602444-null-null.142^v102^pc_search_result_base2&utm_term=KEIL%E6%96%AD%E7%BD%91&spm=1018.2226.3001.4187),先试试这个

---

上面的那个[防火墙方法](https://blog.csdn.net/weixin_65672107/article/details/146602444?ops_request_misc=elastic_search_misc&request_id=94e6a7b409b6baa33dfa2391a317eb78&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~sobaiduend~default-4-146602444-null-null.142^v102^pc_search_result_base2&utm_term=KEIL%E6%96%AD%E7%BD%91&spm=1018.2226.3001.4187)不起作用,可能是因为公司的杀毒软件会接管防火墙,你没权限关杀毒软件(公司要监管)

这时候就只能通过抓ip来封锁了

保存为 **`Keil_Auto_Defense.ps1`**，右键选择 **“以管理员身份运行”**：

```powershell
# ========================================================
# 1. 阶段三：静态黑洞预置（针对已知的固定遥测服务）
# ========================================================
Write-Host ">>> [阶段三] 预置已知固定遥测黑洞路由 (72.52.161.233)..." -ForegroundColor Cyan
route -p add 72.52.161.233 mask 255.255.255.255 127.0.0.1 metric 1 > $null 2>&1

Write-Host ">>> [阶段四] 启动实时动态防御雷达（检测到外连即时报警并封锁）..." -ForegroundColor Green
Write-Host ">>> 提示: 请保持此窗口运行，按 Ctrl + C 可退出`n" -ForegroundColor DarkGray

# 记录本次运行已封锁的 IP 集合，防止同一 IP 连续数秒重复刷屏
$blockedIps = [System.Collections.Generic.HashSet[string]]::new()
[void]$blockedIps.Add("72.52.161.233")

# ========================================================
# 2. 阶段四：动态嗅探 + 报警 + 自动打靶封锁循环
# ========================================================
while ($true) {
    # 查找属于 UV4 或 PackInstaller 的所有外向公网 TCP 连接（排除 0.0.0.0 与本地回环 127.0.0.1）
    $outbound = Get-NetTCPConnection -ErrorAction SilentlyContinue | Where-Object {
        $p = Get-Process -Id $_.OwningProcess -ErrorAction SilentlyContinue
        $p -and ($p.ProcessName -match "UV4|PackInstaller") -and ($_.RemoteAddress -notmatch "^(0\.0\.0\.0|127\.0\.0\.1|::)$")
    }

    if ($outbound) {
        foreach ($conn in $outbound) {
            $targetIp = $conn.RemoteAddress

            # 如果该 IP 尚未封锁或尚未提示
            if (-not $blockedIps.Contains($targetIp)) {
                $now = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
                $procName = (Get-Process -Id $conn.OwningProcess -ErrorAction SilentlyContinue).ProcessName

                # 终端明确告警输出
                Write-Host "`n========================= [ 安全拦截告警 ] =========================" -ForegroundColor Red
                Write-Host "[$now] 警告: 检测到程序正在向外发送数据!" -ForegroundColor Red
                Write-Host "   -> 触发程序 : $procName.exe (PID: $($conn.OwningProcess))" -ForegroundColor Yellow
                Write-Host "   -> 检测到 IP: $targetIp (目标端口: $($conn.RemotePort))" -ForegroundColor Yellow

                # 执行底层精准打靶封锁
                cmd.exe /c "route -p add $targetIp mask 255.255.255.255 127.0.0.1 metric 1" > $null 2>&1
            
                # 终端明确显示封锁结果
                Write-Host "   -> 处置状态 : 检测到该 IP ($targetIp)，已经成功将其封锁并重定向至 127.0.0.1！" -ForegroundColor Green
                Write-Host "====================================================================`n" -ForegroundColor Red

                # 记录已封锁
                [void]$blockedIps.Add($targetIp)

                # 物理蜂鸣音报警
                [Console]::Beep(1200, 300)
                [Console]::Beep(1000, 400)
            }
        }
    } else {
        # 平时安全脱机时，单行动态刷新状态
        $procRunning = Get-Process -Name "UV4", "PackInstaller" -ErrorAction SilentlyContinue
        $runningNames = if ($procRunning) { ($procRunning.ProcessName | Select-Object -Unique) -join ", " } else { "未检测到进程" }
        Write-Host -NoNewline "`r[$([DateTime]::Now.ToString('HH:mm:ss'))] 监控中... 运行中进程: [$runningNames] -> 网络状态: [100% 离线脱机]   "
    }

    Start-Sleep -Seconds 1
}
```

### 实际报警效果预览

一旦 `UV4` 或 `PackInstaller` 发起外连，终端会直接输出如下信息并响铃：

```text
========================= [ 安全拦截告警 ] =========================
[2026-10-16 14:05:22] 警告: 检测到程序正在向外发送数据!
   -> 触发程序 : PackInstaller.exe (PID: 34112)
   -> 检测到 IP: 23.215.7.154 (目标端口: 443)
   -> 处置状态 : 检测到该 IP (23.215.7.154)，已经成功将其封锁并重定向至 127.0.0.1！
====================================================================
```
