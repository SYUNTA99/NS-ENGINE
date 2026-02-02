# Desktop notification (best effort)
try {
    # BurntToast を優先（Git Bash 経由でも安定）
    $bt = Get-Module -ListAvailable -Name BurntToast -ErrorAction SilentlyContinue
    if ($bt) {
        Import-Module BurntToast -ErrorAction Stop
        New-BurntToastNotification -Text "Claude Code", "Claude is waiting"
    } else {
        # WinRT を試す（Git Bash 経由だと失敗しやすい）
        [Windows.UI.Notifications.ToastNotificationManager, Windows.UI.Notifications, ContentType = WindowsRuntime] | Out-Null
        [Windows.Data.Xml.Dom.XmlDocument, Windows.Data.Xml.Dom.XmlDocument, ContentType = WindowsRuntime] | Out-Null

        $template = @"
<toast>
    <visual>
        <binding template="ToastText02">
            <text id="1">Claude Code</text>
            <text id="2">Claude is waiting</text>
        </binding>
    </visual>
</toast>
"@
        $xml = New-Object Windows.Data.Xml.Dom.XmlDocument
        $xml.LoadXml($template)
        $toast = New-Object Windows.UI.Notifications.ToastNotification $xml
        [Windows.UI.Notifications.ToastNotificationManager]::CreateToastNotifier("Claude Code").Show($toast)
    }
} catch {
    # デスクトップ通知が失敗しても ntfy がバックアップ
}

# ntfy push (常に実行)
try {
    Invoke-RestMethod -Uri "https://ntfy.sh/ns-engine-claude-240909" -Method Post -Body "Claude is waiting" -ErrorAction SilentlyContinue
} catch {}

exit 0
