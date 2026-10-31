const API = {
  ranked: q => `/api/search/ranked?q=${encodeURIComponent(q)}`,
  and: q => `/api/search/and?q=${encodeURIComponent(q)}`,
  or: q => `/api/search/or?q=${encodeURIComponent(q)}`,
  suggest: q => `/api/suggest?q=${encodeURIComponent(q)}`,
  benchmark: `/api/benchmark`,
};
 
async function safeJson(res) {
  const contentType = res.headers.get('content-type') || '';
  if (!contentType.includes('application/json')) {
    throw new Error('Server tra ve khong phai JSON — kiem tra da mo dung http://127.0.0.1:5000 chua (khong phai file:// hoac preview).');
  }
  return res.json();
}
 
let currentMode = 'ranked';
// Theo dõi tên file nguồn hiện tại — mặc định là data/document.txt,
// cập nhật lại thành tên file người dùng vừa upload thành công
let currentCorpusFile = 'data/document.txt';
 
const searchBtnLabel = document.getElementById('searchBtnLabel');
const hintText = document.getElementById('hintText');
const HINTS = {
  ranked: 'Kết quả xếp hạng theo điểm TF-IDF.',
  and:    'Nhiều từ khóa cho <code>AND</code>: cách nhau bởi dấu phẩy — vd <code>avl,rotation</code>',
  or:     'Nhiều từ khóa cho <code>OR</code>: cách nhau bởi dấu phẩy — vd <code>avl,hash</code>',
  add:    'Gõ 1 câu/document rồi bấm <b style="color:var(--amber)">Add</b> để thêm vào index ngay lập tức.',
};
 
// ===== Mode toggle =====
document.querySelectorAll('.mode-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    document.querySelectorAll('.mode-btn').forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
    currentMode = btn.dataset.mode;
    searchBtnLabel.textContent = currentMode === 'add' ? 'Add' : 'Search';
    hintText.innerHTML = HINTS[currentMode];
    searchInput.placeholder = currentMode === 'add'
      ? 'gõ nội dung document mới...'
      : 'avl, rotation, balance...';
  });
});
 
// ===== Ambient AVL tree (ornamental, pulses on search) =====
const treeLayout = [
  { id: 0, x: 150, y: 30, parent: null },
  { id: 1, x: 80,  y: 90, parent: 0 },
  { id: 2, x: 220, y: 90, parent: 0 },
  { id: 3, x: 45,  y: 155, parent: 1 },
  { id: 4, x: 115, y: 155, parent: 1 },
  { id: 5, x: 185, y: 155, parent: 2 },
  { id: 6, x: 255, y: 155, parent: 2 },
];
const edgesG = document.getElementById('edges');
const nodesG = document.getElementById('nodes');
 
treeLayout.forEach(n => {
  if (n.parent !== null) {
    const p = treeLayout[n.parent];
    const line = document.createElementNS('http://www.w3.org/2000/svg', 'line');
    line.setAttribute('x1', p.x); line.setAttribute('y1', p.y);
    line.setAttribute('x2', n.x); line.setAttribute('y2', n.y);
    line.setAttribute('class', 'tedge');
    line.dataset.id = n.id;
    edgesG.appendChild(line);
  }
  const circle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
  circle.setAttribute('cx', n.x); circle.setAttribute('cy', n.y); circle.setAttribute('r', 15);
  circle.setAttribute('class', 'tnode');
  circle.dataset.id = n.id;
  nodesG.appendChild(circle);
 
  const label = document.createElementNS('http://www.w3.org/2000/svg', 'text');
  label.setAttribute('x', n.x); label.setAttribute('y', n.y);
  label.setAttribute('class', 'tlabel');
  label.dataset.id = n.id;
  label.textContent = n.id;
  nodesG.appendChild(label);
});
 
function pulseTree() {
  // Mô phỏng 1 lượt duyệt root -> lá, mang tính minh họa (ambient), không phải trace thật của lần search cụ thể
  const paths = [[0,1,3],[0,1,4],[0,2,5],[0,2,6]];
  const path = paths[Math.floor(Math.random() * paths.length)];
  document.querySelectorAll('.tnode, .tedge, .tlabel').forEach(el => el.classList.remove('lit'));
  path.forEach((id, i) => {
    setTimeout(() => {
      document.querySelectorAll(`[data-id="${id}"]`).forEach(el => el.classList.add('lit'));
    }, i * 140);
  });
  setTimeout(() => {
    document.querySelectorAll('.tnode, .tedge, .tlabel').forEach(el => el.classList.remove('lit'));
  }, path.length * 140 + 900);
}
setInterval(pulseTree, 3400);
pulseTree();
 
// ===== Search =====
const searchInput = document.getElementById('searchInput');
const searchBtn = document.getElementById('searchBtn');
const resultList = document.getElementById('resultList');
const emptyState = document.getElementById('emptyState');
const suggestBanner = document.getElementById('suggestBanner');
const suggestList = document.getElementById('suggestList');
const statRow = document.getElementById('statRow');
 
async function runSearch() {
  const q = searchInput.value.trim();
  if (!q) return;
 
  pulseTree();
  suggestBanner.classList.remove('show');
  statRow.innerHTML = `<span>searching…</span>`;
  const t0 = performance.now();
 
  try {
    let docIds = [];
    let ranked = null;
 
    if (currentMode === 'ranked') {
      const res = await fetch(API.ranked(q));
      const data = await safeJson(res);
      if (data.error) throw new Error(data.error);
      ranked = data.results;
      docIds = ranked.map(r => r.docId);
    } else {
      const url = currentMode === 'and' ? API.and(q) : API.or(q);
      const res = await fetch(url);
      const data = await safeJson(res);
      if (data.error) throw new Error(data.error);
      docIds = data.docIds;
    }
 
    const elapsed = (performance.now() - t0).toFixed(1);
    statRow.innerHTML = `
      <span><b>${elapsed}</b> ms round-trip</span>
      <span><b>${docIds.length}</b> results</span>
      <span><b>${currentMode.toUpperCase()}</b> mode</span>
    `;
 
    renderResults(docIds, ranked);
 
    if (docIds.length === 0) {
      const sres = await fetch(API.suggest(q.split(',')[0]));
      const sdata = await safeJson(sres);
      if (sdata.suggestions && sdata.suggestions.length) {
        suggestList.innerHTML = sdata.suggestions.map(s => `<b>${s}</b>`).join(', ');
        suggestBanner.classList.add('show');
      }
    }
  } catch (err) {
    statRow.innerHTML = `<span style="color: var(--danger)">Lỗi: ${err.message}</span>`;
    resultList.innerHTML = '';
    emptyState.classList.remove('show');
  }
}
 
function renderResults(docIds, ranked) {
  resultList.innerHTML = '';
  if (docIds.length === 0) {
    emptyState.textContent = 'Không tìm thấy document nào khớp.';
    emptyState.classList.add('show');
    return;
  }
  emptyState.classList.remove('show');
 
  const maxScore = ranked ? Math.max(...ranked.map(r => r.score), 0.0001) : 1;
 
  docIds.forEach((id, i) => {
    const scoreObj = ranked ? ranked.find(r => r.docId === id) : null;
    const score = scoreObj ? scoreObj.score : null;
    const pct = score !== null ? Math.max(4, (score / maxScore) * 100) : 100;
 
    // Nếu backend trả về sentenceIndex hợp lệ (>= 0), hiện đúng câu chứa từ khóa.
    // Nếu không (mode and/or, hoặc backend chưa hỗ trợ), fallback về dòng gợi ý cũ.
    const sentInfo = (score !== null && scoreObj && scoreObj.sentenceIndex >= 0)
      ? `Câu ${scoreObj.sentenceIndex + 1}: "${scoreObj.sentenceText}"`
      : `Xem chi tiết trong ${currentCorpusFile} tại dòng ${id}`;
 
    const card = document.createElement('div');
    card.className = 'result-card';
    card.style.animationDelay = `${i * 0.04}s`;
    card.innerHTML = `
      <span class="result-rank">${String(i + 1).padStart(2, '0')}</span>
      <div class="result-main">
        <div class="result-title">Paragraph ${id}</div>
        <div class="result-text">${sentInfo}</div>
      </div>
      ${score !== null ? `
      <div class="result-score">
        <div class="score-val">${score.toFixed(4)}</div>
        <div class="score-bar-track"><div class="score-bar-fill" style="width:${pct}%"></div></div>
      </div>` : ''}
    `;
    resultList.appendChild(card);
  });
}
 
// ===== Add document (từ thanh tìm kiếm, mode ADD) =====
const uploadStatus = document.getElementById('uploadStatus');
 
async function addDocument() {
  const text = searchInput.value.trim();
  if (!text) return;
 
  searchBtnLabel.textContent = 'Adding…';
  try {
    const res = await fetch('/api/documents/add', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ text })
    });
    const data = await safeJson(res);
    if (data.error) throw new Error(data.error);
 
    searchInput.value = '';
    statRow.innerHTML = `<span style="color:var(--accent)">Đã thêm document mới ✓</span>`;
    updateIndexMeta(data.stats);
    pulseTree();
  } catch (err) {
    statRow.innerHTML = `<span style="color: var(--danger)">Lỗi: ${err.message}</span>`;
  } finally {
    searchBtnLabel.textContent = 'Add';
  }
}
 
function handleSubmit() {
  if (currentMode === 'add') addDocument();
  else runSearch();
}
 
searchBtn.addEventListener('click', handleSubmit);
searchInput.addEventListener('keydown', e => { if (e.key === 'Enter') handleSubmit(); });
 
// ===== Upload file =====
const fileInput = document.getElementById('fileInput');
const replaceModeCheckbox = document.getElementById('replaceMode');
 
fileInput.addEventListener('change', async () => {
  const file = fileInput.files[0];
  if (!file) return;
 
  const isReplace = replaceModeCheckbox.checked;
 
  // Xác nhận lại nếu chọn Replace — vì thao tác này xóa hết dữ liệu cũ, không hoàn tác được
  if (isReplace) {
    const ok = confirm(
      `Toàn bộ document hiện có sẽ bị XÓA và thay bằng nội dung file "${file.name}".\n` +
      `Thao tác này không thể hoàn tác. Tiếp tục?`
    );
    if (!ok) {
      fileInput.value = '';
      return;
    }
  }
 
  uploadStatus.textContent = 'Đang tải lên…';
  uploadStatus.classList.remove('error');
 
  const formData = new FormData();
  formData.append('file', file);
  formData.append('mode', isReplace ? 'replace' : 'append');
 
  try {
    const res = await fetch('/api/documents/upload', { method: 'POST', body: formData });
    const data = await safeJson(res);
    if (data.error) throw new Error(data.error);
 
    // Cập nhật tên file nguồn để hiển thị đúng trong kết quả search sau này
    currentCorpusFile = file.name;
 
    uploadStatus.textContent = isReplace
      ? `Đã thay thế toàn bộ bằng ${data.added} document từ "${file.name}" ✓`
      : `Đã thêm ${data.added} document từ "${file.name}" ✓`;
    updateIndexMeta(data.stats);
    pulseTree();
  } catch (err) {
    uploadStatus.textContent = `Lỗi: ${err.message}`;
    uploadStatus.classList.add('error');
  } finally {
    fileInput.value = '';
  }
});
 
function updateIndexMeta(stats) {
  if (!stats) return;
  document.getElementById('indexMeta').textContent =
    `${stats.docCount} documents · ${stats.indexSize} từ duy nhất · height ${stats.height}`;
}
 
async function loadStats() {
  try {
    const res = await fetch('/api/stats');
    const data = await safeJson(res);
    if (!data.error) updateIndexMeta(data);
  } catch (_) { /* im lặng nếu chưa sẵn sàng */ }
}
loadStats();
 
// ===== Benchmark (load once on page load) =====
async function loadBenchmark() {
  const grid = document.getElementById('benchGrid');
  const meta = document.getElementById('indexMeta');
  try {
    const res = await fetch(API.benchmark);
    const data = await safeJson(res);
    if (data.error) throw new Error(data.error);
 
    grid.innerHTML = `
      <div class="bench-card is-avl">
        <div class="bench-name">AVL Tree (custom)</div>
        <div class="bench-value">${data.avl.timeMs.toFixed(3)}<small> ms</small></div>
        <div class="bench-note">height ${data.avl.height} · avg ${data.avl.avgSteps.toFixed(1)} steps</div>
      </div>
      <div class="bench-card">
        <div class="bench-name">std::map (Red-Black)</div>
        <div class="bench-value">${data.stdMap.timeMs.toFixed(3)}<small> ms</small></div>
        <div class="bench-note">O(log n) guaranteed</div>
      </div>
      <div class="bench-card">
        <div class="bench-name">unordered_map</div>
        <div class="bench-value">${data.unorderedMap.timeMs.toFixed(3)}<small> ms</small></div>
        <div class="bench-note">O(1) average, no ordering</div>
      </div>
    `;
    meta.textContent = `index size ${data.indexSize} · height ${data.actualHeight} (theory ≈ ${data.theoreticalHeight.toFixed(2)})`;
  } catch (err) {
    grid.innerHTML = `<div class="bench-card"><div class="bench-name" style="color:var(--danger)">Không tải được benchmark: ${err.message}</div></div>`;
  }
}
loadBenchmark();
emptyState.classList.add('show');