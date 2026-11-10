# Aether EdTech website

Static, four-language (EN/FR/NL/DE) website for **Aether EdTech ASBL** (BCE/KBO 1045.010.593), Brussels.

| Path | What |
|---|---|
| `site/` | Generated static site (served from Azure Storage static website, behind Cloudflare) |
| `tools/build.py`, `tools/overrides.py` | Generator: all page text and translations live here |
| `tools/deploy.sh` | Rebuild and upload `site/` to Azure Storage |
| `api/` | Azure Function (Node 20) for the contact form + bilingual FR/EN auto-reply email (Azure Communication Services Email, EU data location) |

## Edit and publish
```sh
python3 tools/build.py      # regenerate site/{en,fr,nl,de}/
sh tools/deploy.sh          # needs: az login
```
Preview locally: `python3 -m http.server 4173 --directory site`

## Azure (resource group `aether`, North Europe)
- Storage account `aetherasblhxdid9` (static website)
- Function App `aether-contact-api`, Communication Services `aether-comm`, Email Service `aether-email`
- App settings (set in Azure, never committed): `ACS_CONNECTION_STRING`, `SENDER_ADDRESS`, `NOTIFY_TO`, `SITE_URL`, `ALLOWED_ORIGINS`

## Email authentication (Cloudflare DNS)
SPF (Google + Azure), DKIM (Google + Azure selector1/2), DMARC `p=none` with no report address.

No secrets belong in this repo.
