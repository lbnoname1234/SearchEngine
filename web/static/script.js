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
      ranked = data.results || [];
      docIds = data.docIds || ranked.map(r => r.docId);
    }
 
    const elapsed = (performance.now() - t0).toFixed(1);
    statRow.innerHTML = `
      <span><b>${elapsed}</b> ms round-trip</span>
      <span><b>${docIds.length}</b> results</span>
      <span><b>${currentMode.toUpperCase()}</b> mode</span>
    `;
 
    renderResults(docIds, ranked, q);
 
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
 
function escapeHtml(str) {
  if (!str) return '';
  return str
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

function escapeRegex(str) {
  return str.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

function highlightKeywords(text, keywords, matchedTokens) {
  let safe = escapeHtml(text);
  const targets = [...new Set([
    ...(matchedTokens || []),
    ...(keywords || [])
  ])].filter(w => w && w.trim().length > 0);

  targets.sort((a, b) => b.length - a.length);

  targets.forEach(t => {
    try {
      const re = new RegExp(`(${escapeRegex(t)})`, 'gi');
      safe = safe.replace(re, '<mark class="kw-hl">$1</mark>');
    } catch (_) {}
  });
  return safe;
}

function renderResults(docIds, items, query) {
  resultList.innerHTML = '';
  if (docIds.length === 0) {
    emptyState.textContent = 'Không tìm thấy document nào khớp.';
    emptyState.classList.add('show');
    return;
  }
  emptyState.classList.remove('show');

  const queryTerms = (query || '').split(',').map(s => s.trim().toLowerCase()).filter(Boolean);
  const hasRanked = items && items.length > 0 && typeof items[0].score === 'number';
  const maxScore = hasRanked ? Math.max(...items.map(r => r.score || 0), 0.0001) : 1;

  docIds.forEach((id, i) => {
    const item = items ? items.find(r => r.docId === id) : null;
    const score = (item && typeof item.score === 'number') ? item.score : null;
    const pct = score !== null ? Math.max(4, (score / maxScore) * 100) : 100;
    const positions = (item && Array.isArray(item.positions)) ? item.positions : [];
    const totalOccurrences = item ? (item.totalOccurrences || positions.length) : 0;

    const sentenceMap = new Map();
    positions.forEach(p => {
      if (!sentenceMap.has(p.sentenceIndex)) {
        sentenceMap.set(p.sentenceIndex, {
          sentenceIndex: p.sentenceIndex,
          snippet: p.snippet,
          matchedTokens: []
        });
      }
      sentenceMap.get(p.sentenceIndex).matchedTokens.push(p.matchedWord);
    });

    let positionsPillsHtml = '';
    if (positions.length > 0) {
      positionsPillsHtml = `
        <div class="pos-pills-row">
          <span class="pos-label">Positions:</span>
          ${positions.map((p, pIdx) => `
            <span class="pos-pill ${p.isExact ? 'exact' : 'element'}" title="${p.isExact ? 'Exact word match' : 'Sub-word/element match'}: '${escapeHtml(p.matchedWord)}'">
              #${pIdx + 1} Sent ${p.sentenceIndex + 1}, Word ${p.wordIndex + 1}
              ${!p.isExact ? `<small class="subword-tag">(in "${escapeHtml(p.matchedWord)}")</small>` : ''}
            </span>
          `).join('')}
        </div>
      `;
    }

    let snippetsHtml = '';
    if (sentenceMap.size > 0) {
      snippetsHtml = `
        <div class="snippets-container">
          ${Array.from(sentenceMap.values()).map(s => {
            const highlighted = highlightKeywords(s.snippet, queryTerms, s.matchedTokens);
            return `
              <div class="snippet-line">
                <span class="sent-num">Sentence ${s.sentenceIndex + 1}:</span>
                <span class="sent-text">"${highlighted}"</span>
              </div>
            `;
          }).join('')}
        </div>
      `;
    } else if (item && item.sentenceIndex >= 0 && item.sentenceText) {
      const highlighted = highlightKeywords(item.sentenceText, queryTerms, []);
      snippetsHtml = `
        <div class="snippets-container">
          <div class="snippet-line">
            <span class="sent-num">Sentence ${item.sentenceIndex + 1}:</span>
            <span class="sent-text">"${highlighted}"</span>
          </div>
        </div>
      `;
    } else {
      snippetsHtml = `<div class="result-text">Xem chi tiết trong ${currentCorpusFile || 'corpus'} tại dòng ${id}</div>`;
    }

    const card = document.createElement('div');
    card.className = 'result-card';
    card.style.animationDelay = `${i * 0.04}s`;
    card.innerHTML = `
      <span class="result-rank">${String(i + 1).padStart(2, '0')}</span>
      <div class="result-main">
        <div class="result-title">
          Paragraph ${id}
          ${totalOccurrences > 0 ? `<span class="occurrence-pill">${totalOccurrences} match${totalOccurrences > 1 ? 'es' : ''}</span>` : ''}
        </div>
        ${positionsPillsHtml}
        ${snippetsHtml}
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
    if (data.files) renderFileList(data.files);
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
    if (data.files) renderFileList(data.files);
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
 
// ===== File Management: Current Files & Removal =====
const fileChipsContainer = document.getElementById('fileChipsContainer');
const clearAllFilesBtn = document.getElementById('clearAllFilesBtn');

function renderFileList(files) {
  if (!fileChipsContainer) return;
  fileChipsContainer.innerHTML = '';

  if (!files || files.length === 0) {
    fileChipsContainer.innerHTML = '<span class="empty-files-msg">No files imported. Upload a .txt file to index.</span>';
    if (clearAllFilesBtn) clearAllFilesBtn.style.display = 'none';
    currentCorpusFile = '';
    return;
  }

  if (clearAllFilesBtn) clearAllFilesBtn.style.display = 'inline-block';
  currentCorpusFile = files.map(f => f.filename).join(', ');

  files.forEach(f => {
    const chip = document.createElement('div');
    chip.className = 'file-chip';
    chip.innerHTML = `
      <span class="file-chip-icon">📄</span>
      <span class="file-chip-name" title="${f.filename}">${f.filename}</span>
      <span class="file-chip-badge">${f.docCount} docs</span>
      <button class="file-chip-del" data-filename="${f.filename}" title="Remove ${f.filename}">✕</button>
    `;

    chip.querySelector('.file-chip-del').addEventListener('click', async (e) => {
      e.stopPropagation();
      const fname = f.filename;
      if (!confirm(`Are you sure you want to remove "${fname}" from the search index?`)) return;

      try {
        const res = await fetch('/api/documents/remove', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ filename: fname })
        });
        const data = await safeJson(res);
        if (data.error) throw new Error(data.error);

        renderFileList(data.files);
        updateIndexMeta(data.stats);
        pulseTree();
        uploadStatus.textContent = `Removed file "${fname}" ✓`;
        uploadStatus.classList.remove('error');

        if (data.stats && data.stats.docCount === 0) {
          resultList.innerHTML = '';
          emptyState.textContent = 'Index is now empty. Upload a .txt file to search.';
          emptyState.classList.add('show');
        } else if (searchInput.value.trim() && currentMode !== 'add') {
          runSearch();
        }
      } catch (err) {
        uploadStatus.textContent = `Lỗi xóa file: ${err.message}`;
        uploadStatus.classList.add('error');
      }
    });

    fileChipsContainer.appendChild(chip);
  });
}

if (clearAllFilesBtn) {
  clearAllFilesBtn.addEventListener('click', async () => {
    if (!confirm('Are you sure you want to remove ALL imported files and clear the search index?')) return;

    try {
      const res = await fetch('/api/documents/clear', { method: 'POST' });
      const data = await safeJson(res);
      if (data.error) throw new Error(data.error);

      renderFileList([]);
      updateIndexMeta(data.stats);
      pulseTree();
      uploadStatus.textContent = 'Cleared all files from index ✓';
      uploadStatus.classList.remove('error');
      resultList.innerHTML = '';
      emptyState.textContent = 'Index is empty. Upload a .txt file to begin.';
      emptyState.classList.add('show');
    } catch (err) {
      uploadStatus.textContent = `Lỗi xóa toàn bộ: ${err.message}`;
      uploadStatus.classList.add('error');
    }
  });
}

async function loadFiles() {
  try {
    const res = await fetch('/api/documents/files');
    const data = await safeJson(res);
    if (!data.error && data.files) {
      renderFileList(data.files);
    }
  } catch (_) { }
}

async function loadStats() {
  try {
    const res = await fetch('/api/stats');
    const data = await safeJson(res);
    if (!data.error) updateIndexMeta(data);
    loadFiles();
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