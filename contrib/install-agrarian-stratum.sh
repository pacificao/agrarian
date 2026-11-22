#!/usr/bin/env bash
# Apply files prepared by prepare-agrarian-stratum.py on the production host.
# Run as root, passing the private backup directory and the health-check script.
set -Eeuo pipefail
[[ $EUID == 0 && $# == 2 ]]
stage=$(realpath -e -- "$1")
probe=$(realpath -e -- "$2")
[[ "$stage" =~ ^/root/agrarian-stratum-rollout\.[[:alnum:]]{8}$ ]]
[[ -d "$stage" && -f "$probe" ]]
[[ "$(stat -c '%u:%a' "$stage")" == 0:700 ]]
for file in miningcore.before miningcore.after nginx.before nginx.after stream.conf certbot-hook; do
    [[ -f "$stage/$file" && ! -L "$stage/$file" ]]
done

pool=/etc/miningcore/agrarian.config.json
nginx=/etc/nginx/nginx.conf
include=/etc/nginx/agrarian-stratum.conf
hook=/etc/letsencrypt/renewal-hooks/deploy/agrarian-stratum-nginx-reload
cmp -s "$stage/miningcore.before" "$pool"
cmp -s "$stage/nginx.before" "$nginx"
[[ ! -e "$include" && ! -e "$hook" ]]
[[ -z "$(ss -H -lt '( sport = :3033 or sport = :3034 )')" ]]
firewall=$(ufw status)
[[ "$firewall" == "Status: active"* && "$firewall" != *3033* ]]
systemctl is-active --quiet nginx miningcore-agrarian
curl --fail --silent --max-time 10 https://alltheacronyms.com/ -o /dev/null
curl --fail --silent --max-time 10 https://agrariancoin.com/ -o /dev/null

pool_changed=0
nginx_changed=0
firewall_changed=0
rollback() {
    trap - ERR
    set +e
    echo "TLS startup check failed; restoring prior configuration from $stage." >&2
    if ((firewall_changed)); then ufw --force delete allow 3033/tcp; fi
    if [[ -e "$hook" ]]; then mv -- "$hook" "$stage/certbot-hook.disabled"; fi
    if ((nginx_changed)); then
        install -m 644 -o root -g root -- "$stage/nginx.before" "$nginx"
        if [[ -e "$include" ]]; then mv -- "$include" "$stage/stream.disabled"; fi
        nginx -t && systemctl reload nginx
    fi
    if ((pool_changed)); then
        install -m 640 -o root -g www-data -- "$stage/miningcore.before" "$pool"
        systemctl restart miningcore-agrarian
    fi
    exit 1
}
trap rollback ERR
nginx_changed=1
install -m 644 -o root -g root -- "$stage/stream.conf" "$include"
install -m 644 -o root -g root -- "$stage/nginx.after" "$nginx"
nginx -t
pool_changed=1
install -m 640 -o root -g www-data -- "$stage/miningcore.after" "$pool"
systemctl restart miningcore-agrarian
systemctl reload nginx
for attempt in {1..12}; do
    if python3 "$probe" --local; then break; fi
    [[ $attempt != 12 ]]
    sleep 2
done
curl --fail --silent --max-time 10 https://alltheacronyms.com/ -o /dev/null
curl --fail --silent --max-time 10 https://agrariancoin.com/ -o /dev/null
install -m 755 -o root -g root -- "$stage/certbot-hook" "$hook"
ufw allow 3033/tcp comment 'Agrarian wallet Stratum TLS'
firewall_changed=1
systemctl is-active --quiet nginx miningcore-agrarian
trap - ERR
echo "TLS active on port 3033; rollback files retained in $stage."
