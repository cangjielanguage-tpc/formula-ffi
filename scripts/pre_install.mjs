// pre_install.mjs - 根项目预处理脚本
// 1. 初始化 git submodule (formula-ffi)
// 2. 触发 stdx 下载 (formula-ffi/scripts/pre_stdx_install.mjs)
import { execSync } from 'child_process';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = path.resolve(__dirname, '..');
const SUBMODULE_PATH = path.join(PROJECT_ROOT, 'formula-ffi');
const STDX_SCRIPT = path.join(SUBMODULE_PATH, 'scripts', 'pre_stdx_install.mjs');

// 步骤 1: 检查并初始化 git submodule
function ensureSubmodule() {
  // 检查 .gitmodules 是否存在
  const gitmodulesPath = path.join(PROJECT_ROOT, '.gitmodules');

  if (!fs.existsSync(gitmodulesPath)) {
    console.log('No .gitmodules found, skipping submodule init.');
    return;
  }

  // 检查 submodule 是否已初始化
  let submoduleReady = false;
  try {
    const result = execSync('git submodule status', {
      cwd: PROJECT_ROOT,
      encoding: 'utf-8',
      stdio: ['pipe', 'pipe', 'pipe']
    }).trim();
    // 如果状态行不以 '-' 开头，表示已初始化
    submoduleReady = result.length > 0 && !result.startsWith('-');
  } catch (e) {
    // git 命令可能失败（如非 git 仓库），忽略
  }

  if (!submoduleReady) {
    console.log('Initializing git submodule: formula-ffi ...');
    try {
      execSync('git submodule update --init --recursive', {
        cwd: PROJECT_ROOT,
        stdio: 'inherit'
      });
      console.log('Submodule initialized successfully.');
    } catch (error) {
      console.error('Failed to initialize submodule:', error.message);
      process.exit(1);
    }
  } else {
    console.log('Submodule formula-ffi already initialized, skipping.');
  }
}

// 步骤 2: 触发 stdx 下载
function downloadStdx() {
  if (!fs.existsSync(STDX_SCRIPT)) {
    console.error('pre_stdx_install.mjs not found at:', STDX_SCRIPT);
    console.error('Please ensure the submodule is properly initialized.');
    process.exit(1);
  }

  console.log('Running pre_stdx_install.mjs ...');
  try {
    execSync(`node "${STDX_SCRIPT}"`, {
      cwd: SUBMODULE_PATH,
      stdio: 'inherit'
    });
    console.log('stdx download completed.');
  } catch (error) {
    console.error('Failed to download stdx:', error.message);
    process.exit(1);
  }
}

// 执行
ensureSubmodule();
downloadStdx();
