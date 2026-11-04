// scripts/pre_har_install.mjs
// preInstall hook 脚本：下载 formula.har 预编译包并做 sha256 校验
// 说明：
// - 使用固定直链下载（gitcode release API 存在共享账号限流问题，HTTP 429），
//   完整性由脚本内固定的 sha256 摘要保证。升级 formula 版本时需同步更新
//   RELEASE_TAG / HAR_URL / HAR_SHA256。
// - 下载到临时文件、校验通过后原子重命名，避免并发执行时写坏正式文件；
//   缓存命中时复验 sha256，损坏缓存会自动重新下载。
import { execFileSync } from 'child_process';
import crypto from 'crypto';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = path.resolve(__dirname, '..');

// formula har release 配置（v2.0.0 附件 formula_6.1.1-v2.0.0.har）
const RELEASE_TAG = 'v2.0.0';
const HAR_URL = `https://gitcode.com/Cangjie-TPC/formula-ffi/releases/download/${RELEASE_TAG}/formula_6.1.1-${RELEASE_TAG}.har`;
const HAR_SHA256 = '347e16261ca26c93fcac1b00502d936f78e6f3f1844c23d2e358506a429b7c2e';
const HAR_DIR = path.join(PROJECT_ROOT, 'formula_hybrid', 'har');
const HAR_PATH = path.join(HAR_DIR, 'formula.har');
const TAG_MARKER_PATH = path.join(HAR_DIR, 'formula.har.tag');
const LOG_PATH = path.join(HAR_DIR, 'pre_har_install.log');

function sha256(filePath) {
    return crypto.createHash('sha256').update(fs.readFileSync(filePath)).digest('hex');
}

// 日志写入失败不影响主流程（避免掩盖原始错误）
function logDownload(result, fields) {
    try {
        const line = `[${new Date().toISOString()}] ${result} ${Object.entries(fields)
            .map(([key, value]) => `${key}=${value}`)
            .join(' ')}\n`;
        fs.mkdirSync(HAR_DIR, { recursive: true });
        fs.appendFileSync(LOG_PATH, line);
    } catch (error) {
        console.error(`[pre_har_install] Failed to write download log: ${error.message}`);
    }
}

function clearCache() {
    for (const filePath of [HAR_PATH, TAG_MARKER_PATH]) {
        if (fs.existsSync(filePath)) {
            fs.unlinkSync(filePath);
        }
    }
}

// 缓存有效性 = tag 标记匹配 + sha256 复验通过；
// 摘要不匹配说明缓存损坏/被篡改，清除后走重新下载分支
function isValidCachedHar() {
    if (!fs.existsSync(HAR_PATH) || !fs.existsSync(TAG_MARKER_PATH)
        || fs.readFileSync(TAG_MARKER_PATH, 'utf8').trim() !== RELEASE_TAG) {
        return false;
    }
    const actual = sha256(HAR_PATH);
    if (actual !== HAR_SHA256) {
        console.log(`Cached formula.har sha256 mismatch (expected ${HAR_SHA256}, got ${actual}), re-downloading.`);
        logDownload('FAIL', { tag: RELEASE_TAG, error: 'cache-sha256-mismatch', actual });
        return false;
    }
    return true;
}

// 下载到临时文件，校验通过后原子重命名；curl 不经 shell 拼接，
// 配置连接/整体超时与重试间隔，失败时记录 curl 退出码
function downloadHar() {
    fs.mkdirSync(HAR_DIR, { recursive: true });
    const tmpPath = `${HAR_PATH}.tmp.${process.pid}`;

    console.log(`Downloading formula.har from ${HAR_URL} ...`);
    try {
        execFileSync('curl', [
            '-fL',
            '--retry', '3',
            '--retry-delay', '2',
            '--connect-timeout', '10',
            '--max-time', '300',
            '-o', tmpPath,
            HAR_URL,
        ], { stdio: 'inherit', cwd: PROJECT_ROOT });
    } catch (error) {
        console.error(`curl exited with code ${error.status}`);
        logDownload('FAIL', { tag: RELEASE_TAG, url: HAR_URL, error: `curl-exit-${error.status}` });
        throw error;
    }

    try {
        if (!fs.existsSync(tmpPath) || fs.statSync(tmpPath).size === 0) {
            logDownload('FAIL', { tag: RELEASE_TAG, url: HAR_URL, error: 'empty-or-missing-file' });
            throw new Error('formula.har download failed: file is empty or missing');
        }

        const actualSha256 = sha256(tmpPath);
        if (actualSha256 !== HAR_SHA256) {
            logDownload('FAIL', { tag: RELEASE_TAG, url: HAR_URL, expected: HAR_SHA256, actual: actualSha256, error: 'sha256-mismatch' });
            throw new Error(`formula.har sha256 mismatch, expected ${HAR_SHA256}, got ${actualSha256}`);
        }

        // 校验通过后原子重命名，再写 tag 标记
        fs.renameSync(tmpPath, HAR_PATH);
        fs.writeFileSync(TAG_MARKER_PATH, RELEASE_TAG);

        console.log('formula.har sha256 verified');
        logDownload('OK', { tag: RELEASE_TAG, url: HAR_URL, size: `${fs.statSync(HAR_PATH).size}`, expected: HAR_SHA256, actual: actualSha256, result: 'verified' });
    } finally {
        // 清理残留临时文件（校验失败或异常时）
        if (fs.existsSync(tmpPath)) {
            fs.unlinkSync(tmpPath);
        }
    }
}

// ── 执行 ──────────────────────────────────────────────────
if (isValidCachedHar()) {
    console.log(`formula.har for release ${RELEASE_TAG} already cached and verified, skipping.`);
} else {
    console.log('Downloading required files...');
    clearCache();
    try {
        downloadHar();
    } catch (error) {
        console.error(`Download failed: ${error.message}`);
        clearCache();
        process.exit(1);
    }
}
