#!/bin/sh
cd "$(dirname "$0")" || exit 1
export LD_LIBRARY_PATH=/rom/usr/trimui/lib:/usr/lib
umask 077
# Only one launcher may own the screen. Child services do not inherit this lock.
exec 9>/tmp/gm-native-ui.lock
flock -n 9 || exit 0
[ -z "$(pidof gm-native)" ] || exit 0
menu_file=/tmp/gm-paused-menu-$$
: > "$menu_file"
pause_menu(){
 for menu_pid in $(pidof MainUI); do
  [ "$(cat /proc/$menu_pid/comm 2>/dev/null)" = MainUI ] || continue
  grep -qx "$menu_pid" "$menu_file" || echo "$menu_pid" >> "$menu_file"
  kill -STOP "$menu_pid" 2>/dev/null
 done
}
cleanup(){
 trap - EXIT INT TERM
 if [ -n "$native_pid" ]; then kill -TERM "$native_pid" 2>/dev/null; wait "$native_pid" 2>/dev/null; fi
 [ -z "$guard_pid" ] || kill "$guard_pid" 2>/dev/null
 [ -z "$fetch_pid" ] || kill "$fetch_pid" 2>/dev/null
 [ -z "$dashboard_pid" ] || kill "$dashboard_pid" 2>/dev/null
 for menu_pid in $(sort -u "$menu_file"); do
  [ "$(cat /proc/$menu_pid/comm 2>/dev/null)" != MainUI ] || kill -CONT "$menu_pid" 2>/dev/null
 done
 rm -f "$menu_file"
}
trap cleanup EXIT
trap 'exit 0' INT TERM
pause_menu
mkdir -p recordings drafts
[ -s endpoint.txt ] || printf '%s\n' 'https://127.0.0.1:7831' > endpoint.txt
dashboard_loop(){
 while :; do
  if curl --config auth.conf --silent --show-error --fail --connect-timeout 2 --max-time 8 --output dashboard.tmp "$(cat endpoint.txt)/dashboard.tsv"; then
   if [ "$(head -n 1 dashboard.tmp)" = GM_DASHBOARD_V1 ]; then mv dashboard.tmp dashboard.tsv; fi
  fi
  sleep 10
 done
}
network_status(){
 printf '%s\n' "$1" > connection-status.tmp
 mv connection-status.tmp connection-status.txt
}
find_computer(){
 network_status '正在搜索已配对电脑…'
 if ./gm-discover > endpoint.discovered && grep -Eq '^https://[0-9.]+:7831$' endpoint.discovered; then
  # An address is never trusted until the existing TLS pin and bearer verify it.
  if curl --config auth.conf --fail --silent --connect-timeout 2 --max-time 4 "$(cat endpoint.discovered)/health" > connection-health.tmp; then
   mv endpoint.discovered endpoint.txt
   return 0
  fi
 fi
 return 1
}
fetch_loop(){
 while :; do
  if [ "$(cat /sys/class/net/wlan0/carrier 2>/dev/null)" != 1 ] || ! ip -4 addr show wlan0 2>/dev/null | grep -q 'inet '; then
   network_status '掌机 Wi-Fi 未取得地址 · SELECT 退出后检查系统 Wi-Fi'
   rm -f reconnect.request
   sleep 2
   continue
  fi
  if [ -f reconnect.request ]; then
   rm -f reconnect.request
   find_computer || network_status '未发现电脑 · 正在尝试上次地址'
  fi
  endpoint=$(cat endpoint.txt)
  selected=$(cat selected.id 2>/dev/null | tr -cd 'a-zA-Z0-9-')
  if curl --config auth.conf --silent --show-error --fail --connect-timeout 2 --max-time 12 --output sessions.tmp --get --data-urlencode "selected=$selected" "$endpoint/sessions.tsv"; then
   if [ "$(head -n 1 sessions.tmp)" = GM_NATIVE_V4 ]; then
    mv sessions.tmp sessions.tsv
    if sed -n '2p' sessions.tsv | grep -q '^online'; then
     network_status '已连接电脑和 Codex · 会话同步正常'
    else
     network_status '电脑服务已连接 · Codex 未连接，请在电脑打开 Codex'
    fi
   else
    network_status '电脑响应格式异常 · 按 A 重新连接'
   fi
  else
   failure=$?
   if [ "$failure" = 60 ] || [ "$failure" = 90 ] || [ "$failure" = 22 ]; then
    network_status '配对或服务校验失败 · 需要在电脑检查配对'
   elif find_computer; then
    network_status '已找到电脑 · 正在恢复会话同步'
   elif curl --config auth.conf --fail --silent --connect-timeout 2 --max-time 4 "$endpoint/health" > connection-health.tmp; then
    network_status '电脑服务可达 · Codex 同步超时，正在重试'
   else
    network_status '电脑服务不可达 · 检查电脑唤醒、同一 Wi-Fi 和伴随服务'
   fi
  fi
  sleep 2
 done
}
(trap - EXIT INT TERM; fetch_loop) 9>&- 2>network.log &
fetch_pid=$!
(trap - EXIT INT TERM; dashboard_loop) 9>&- 2>dashboard.log &
dashboard_pid=$!
./gm-native "$@" 9>&- 2>native.log &
native_pid=$!
# A system-menu supervisor may restart MainUI after an ADB launch.
(trap - EXIT INT TERM; while kill -0 "$native_pid" 2>/dev/null; do pause_menu; sleep 1; done) 9>&- &
guard_pid=$!
wait "$native_pid"
