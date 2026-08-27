// pre_install.mjs - 根项目预处理脚本
// 1. 初始化 git submodule (formula-ffi，含预编译 .so)
// 2. 触发 stdx 下载 (formula-ffi/scripts/pre_stdx_install.mjs)
// 3. 复制 formula 的 resfile 资源到 formula_hybrid
import { execSync } from 'child_process';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = path.resolve(__dirname, '..');
const SUBMODULE_PATH = path.join(PROJECT_ROOT, 'formula-ffi');
const STDX_SCRIPT = path.join(SUBMODULE_PATH, 'scripts', 'pre_stdx_install.mjs');
const FORMULA_RES_PATH = path.join(SUBMODULE_PATH, 'formula', 'src', 'main', 'resources', 'resfile');
const HYBRID_RES_PATH = path.join(PROJECT_ROOT, 'formula_hybrid', 'src', 'main', 'resources', 'resfile');
const FORMULA_LIBS_PATH = path.join(SUBMODULE_PATH, 'formula', 'libs');
const ABIS = ['arm64-v8a', 'x86_64'];

// 步骤 1: 检查并初始化 git submodule
function ensureSubmodule() {
  const gitmodulesPath = path.join(PROJECT_ROOT, '.gitmodules');

  if (!fs.existsSync(gitmodulesPath)) {
    console.log('No .gitmodules found, skipping submodule init.');
    return;
  }

  let submoduleReady = false;
  try {
    const result = execSync('git submodule status', {
      cwd: PROJECT_ROOT,
      encoding: 'utf-8',
      stdio: ['pipe', 'pipe', 'pipe']
    }).trim();
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

// 步骤 3: 复制 formula 的 resfile 资源到 formula_hybrid
function copyResources() {
  if (!fs.existsSync(FORMULA_RES_PATH)) {
    console.error('Formula resfile not found at:', FORMULA_RES_PATH);
    console.error('Please ensure the submodule is properly initialized.');
    process.exit(1);
  }

  fs.mkdirSync(HYBRID_RES_PATH, { recursive: true });

  try {
    fs.cpSync(FORMULA_RES_PATH, HYBRID_RES_PATH, { recursive: true, force: true });
    console.log('resfile resources copied to formula_hybrid.');
  } catch (error) {
    console.error('Failed to copy resfile resources:', error.message);
    process.exit(1);
  }
}

// 步骤 4: 检查预编译 .so 是否存在
function verifyNativeLibs() {
  for (const abi of ABIS) {
    const soPath = path.join(FORMULA_LIBS_PATH, abi, 'liblatex.so');
    if (!fs.existsSync(soPath)) {
      console.warn(`Warning: liblatex.so not found in formula/libs/${abi}/`);
      console.warn('Please ensure the submodule is properly initialized with pre-built .so files.');
      return;
    }
  }
  console.log('Native libs (liblatex.so) verified in formula/libs/.');
}

// 执行
ensureSubmodule();
downloadStdx();
copyResources();
verifyNativeLibs();