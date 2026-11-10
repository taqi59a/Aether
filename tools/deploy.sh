#!/bin/sh
# Rebuild and upload the site to Azure (resource group "aether", North Europe).
set -e
cd "$(dirname "$0")/.."
SA=aetherasblhxdid9
python3 tools/build.py
KEY=$(az storage account keys list -n $SA -g aether --query "[0].value" -o tsv)
az storage blob upload-batch --account-name $SA --account-key "$KEY" -d '$web' -s site --overwrite -o none
echo "Deployed: https://$SA.z16.web.core.windows.net/"
