// scripts/pre_har_install.mjs
import { execSync } from 'child_process';
import crypto from 'crypto';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = path.resolve(__dirname, '..');

// formula har release configuration
const RELEASE_TAG = 'v2.0.0';
const RELEASE_API = `https://gitcode.com/api/v5/repos/Cangjie-TPC/formula-ffi/releases/tags/${RELEASE_TAG}`;
const HAR_DIR = path.join(PROJECT_ROOT, 'formula_hybrid', 'har');
const HAR_PATH = path.join(HAR_DIR, 'formula.har');
const TAG_MARKER_PATH = path.join(HAR_DIR, 'formula.har.tag');
const LOG_PATH = path.join(HAR_DIR, 'pre_har_install.log');

// The cached har is valid only when it was downloaded for the current release tag,
// so bumping RELEASE_TAG automatically triggers a fresh download.
function isCachedForCurrentTag() {
    return fs.existsSync(HAR_PATH) && fs.existsSync(TAG_MARKER_PATH)
        && fs.readFileSync(TAG_MARKER_PATH, 'utf8').trim() === RELEASE_TAG;
}

function sha256(filePath) {
    return crypto.createHash('sha256').update(fs.readFileSync(filePath)).digest('hex');
}

// Append one download record line to the log file, e.g.
// [2026-10-30T08:15:23.456Z] OK tag=v2.0.0 asset=formula_6.1.1-v2.0.0.har url=... size=7281165 expected=d328... actual=d328...
function logDownload(result, fields) {
    const line = `[${new Date().toISOString()}] ${result} ${Object.entries(fields)
        .map(([key, value]) => `${key}=${value}`)
        .join(' ')}\n`;
    fs.mkdirSync(HAR_DIR, { recursive: true });
    fs.appendFileSync(LOG_PATH, line);
}

// Resolve the download url and sha256 digest of the .har attach asset from the release API
function resolveHarAsset() {
    const json = execSync(`curl -sfL "${RELEASE_API}"`, { cwd: PROJECT_ROOT }).toString();
    const release = JSON.parse(json);
    const assets = (release.assets || []).filter(a => a.type === 'attach' && a.name.endsWith('.har'));
    if (assets.length !== 1) {
        throw new Error(`expected exactly one .har attach asset, found ${assets.length}`);
    }
    const digest = (assets[0].digest || '').replace(/^sha256:/, '');
    if (!/^[0-9a-f]{64}$/.test(digest)) {
        throw new Error(`asset ${assets[0].name} has no valid sha256 digest`);
    }
    return { name: assets[0].name, url: assets[0].browser_download_url, sha256: digest };
}

// Download formula har when not cached for the current release tag
if (!isCachedForCurrentTag()) {
  console.log(`Downloading required files...`);

  // Resolve download url and expected sha256 from the release API
  let asset;
  try {
      asset = resolveHarAsset();
  } catch (error) {
      console.error(`Failed to resolve har asset from release ${RELEASE_TAG}: ${error.message}`);
      logDownload('FAIL', { tag: RELEASE_TAG, error: `resolve: ${String(error.message).replace(/\s+/g, ' ')}` });
      process.exit(1);
  }

  try {
      // Create directory if it doesn't exist
      fs.mkdirSync(HAR_DIR, { recursive: true });

      // Download formula har (-f makes curl fail on HTTP errors instead of
      // saving an error page, --retry covers transient network failures)
      console.log(`Downloading ${asset.name} from ${asset.url} ...`);
      execSync(`curl -fL --retry 3 -o "${HAR_PATH}" "${asset.url}"`, {
          stdio: 'inherit',
          cwd: PROJECT_ROOT
      });

      // Check if file exists and has content
      if (fs.existsSync(HAR_PATH)) {
          const stats = fs.statSync(HAR_PATH);
          if (stats.size === 0) {
              console.error(`formula.har file is empty, download failed`);
              logDownload('FAIL', { tag: RELEASE_TAG, url: asset.url, error: 'empty-file' });
              fs.unlinkSync(HAR_PATH);
              process.exit(1);
          }
      } else {
          console.error(`formula.har file not found`);
          process.exit(1);
      }

      // Verify sha256 of the downloaded file against the release digest
      const actualSha256 = sha256(HAR_PATH);
      if (actualSha256 !== asset.sha256) {
          console.error(`formula.har sha256 mismatch, expected ${asset.sha256}, got ${actualSha256}`);
          logDownload('FAIL', { tag: RELEASE_TAG, asset: asset.name, url: asset.url, expected: asset.sha256, actual: actualSha256, error: 'sha256-mismatch' });
          fs.unlinkSync(HAR_PATH);
          process.exit(1);
      }

      console.log(`formula.har sha256 verified`);
      console.log(`formula.har downloaded successfully`);
      logDownload('OK', { tag: RELEASE_TAG, asset: asset.name, url: asset.url, size: `${fs.statSync(HAR_PATH).size}`, expected: asset.sha256, actual: actualSha256, result: 'verified' });
      fs.writeFileSync(TAG_MARKER_PATH, RELEASE_TAG);

  } catch (error) {
      console.error(`Download failed: ${error.message}`);
      logDownload('FAIL', { tag: RELEASE_TAG, error: `download: ${String(error.message).replace(/\s+/g, ' ')}` });
      // Clean up if partially downloaded
      if (fs.existsSync(HAR_PATH)) {
          fs.unlinkSync(HAR_PATH);
      }
      process.exit(1);
  }
} else {
  console.log(`formula.har for release ${RELEASE_TAG} already cached, skipping.`);
}
