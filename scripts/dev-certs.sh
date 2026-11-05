#!/usr/bin/env bash
set -euo pipefail
# For local development only. Do not commit private keys or reuse the CA in production.
cd "$(dirname "$0")/.."
mkdir -p certs
if [[ ! -f certs/ca.pem || ! -f certs/ca.key ]]; then
  # A new CA invalidates previously issued development certificates.
  rm -f certs/alpha.pem certs/alpha.key certs/beta.pem certs/beta.key
  openssl req -x509 -newkey rsa:3072 -nodes -days 30 \
    -keyout certs/ca.key -out certs/ca.pem -subj '/CN=Stubnet Development CA' >/dev/null 2>&1
fi
for node in alpha beta; do
  # Preserve identities across repeated runs (including two simultaneous demos).
  if [[ -f "certs/$node.pem" && -f "certs/$node.key" ]]; then
    continue
  fi
  openssl req -newkey rsa:2048 -nodes -keyout "certs/$node.key" \
    -out "certs/$node.csr" -subj "/CN=$node" >/dev/null 2>&1
  printf 'basicConstraints=CA:FALSE\nkeyUsage=digitalSignature,keyEncipherment\nextendedKeyUsage=serverAuth,clientAuth\n' > "certs/$node.ext"
  openssl x509 -req -in "certs/$node.csr" -CA certs/ca.pem -CAkey certs/ca.key \
    -CAcreateserial -out "certs/$node.pem" -days 30 -sha256 \
    -extfile "certs/$node.ext" >/dev/null 2>&1
  rm -f "certs/$node.csr" "certs/$node.ext"
done
chmod 600 certs/*.key
printf 'Generated development certificates in certs/ (DO NOT COMMIT).\n'
