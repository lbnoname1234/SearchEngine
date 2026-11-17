import subprocess
import json
import os
from flask import Flask, jsonify, request, render_template

app = Flask(__name__)
app.config["TEMPLATES_AUTO_RELOAD"] = True
app.config["SEND_FILE_MAX_AGE_DEFAULT"] = 0

# Đường dẫn tới binary đã build từ main_cli.cpp
# Tự động chọn search_cli.exe (Windows) hoặc search_cli (Linux/Render)
_base_dir = os.path.dirname(os.path.abspath(__file__))
_win_cli = os.path.join(_base_dir, "search_cli.exe")
_linux_cli = os.path.join(_base_dir, "search_cli")
CLI_PATH = _win_cli if os.path.isfile(_win_cli) else _linux_cli


def call_cpp(mode: str, keyword: str = ""):
    """Gọi search_cli.exe, parse JSON trả về. Raise lỗi rõ ràng nếu có vấn đề."""
    args = [CLI_PATH, mode]
    if keyword:
        args.append(keyword)

    try:
        result = subprocess.run(
            args,
            capture_output=True,
            text=True,
            cwd=os.path.dirname(CLI_PATH),
            timeout=5  # tránh treo server nếu C++ bị lỗi vòng lặp vô hạn
        )
    except FileNotFoundError:
        raise RuntimeError(f"Khong tim thay {CLI_PATH}. Da build search_cli.exe chua?")
    except subprocess.TimeoutExpired:
        raise RuntimeError("search_cli.exe chay qua lau (timeout).")

    if result.returncode != 0:
        raise RuntimeError(f"search_cli.exe loi: {result.stderr.strip()}")

    if not result.stdout.strip():
        raise RuntimeError("search_cli.exe khong tra ve gi (stdout rong).")

    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError:
        raise RuntimeError(f"Output khong phai JSON hop le: {result.stdout[:200]}")


def error_response(e: Exception, status: int = 500):
    return jsonify({"error": str(e)}), status


# ===== Trang chủ =====
@app.route("/")
def index():
    return render_template("index.html")


# ===== API endpoints, khớp với các mode trong main_cli.cpp =====

@app.route("/api/search/basic")
def search_basic():
    keyword = request.args.get("q", "").strip()
    if not keyword:
        return error_response(ValueError("Thieu tham so q"), 400)
    try:
        data = call_cpp("basic", keyword)
        doc_ids = [d["docId"] if isinstance(d, dict) else d for d in data]
        return jsonify({"docIds": doc_ids, "results": data})
    except RuntimeError as e:
        return error_response(e)


@app.route("/api/search/ranked")
def search_ranked():
    keyword = request.args.get("q", "").strip()
    if not keyword:
        return error_response(ValueError("Thieu tham so q"), 400)
    try:
        data = call_cpp("ranked", keyword)
        doc_ids = [d["docId"] if isinstance(d, dict) else d for d in data]
        return jsonify({"docIds": doc_ids, "results": data})
    except RuntimeError as e:
        return error_response(e)


@app.route("/api/search/and")
def search_and():
    keywords = request.args.get("q", "").strip()  # dạng "avl,rotation"
    if not keywords:
        return error_response(ValueError("Thieu tham so q"), 400)
    try:
        data = call_cpp("and", keywords)
        doc_ids = [d["docId"] if isinstance(d, dict) else d for d in data]
        return jsonify({"docIds": doc_ids, "results": data})
    except RuntimeError as e:
        return error_response(e)


@app.route("/api/search/or")
def search_or():
    keywords = request.args.get("q", "").strip()
    if not keywords:
        return error_response(ValueError("Thieu tham so q"), 400)
    try:
        data = call_cpp("or", keywords)
        doc_ids = [d["docId"] if isinstance(d, dict) else d for d in data]
        return jsonify({"docIds": doc_ids, "results": data})
    except RuntimeError as e:
        return error_response(e)


@app.route("/api/suggest")
def suggest():
    keyword = request.args.get("q", "").strip()
    if not keyword:
        return error_response(ValueError("Thieu tham so q"), 400)
    try:
        data = call_cpp("suggest", keyword)
        return jsonify({"suggestions": data})
    except RuntimeError as e:
        return error_response(e)


@app.route("/api/benchmark")
def benchmark():
    # Cho phép truyền từ khóa tùy chỉnh: /api/benchmark?q=avl,tree,search
    keywords = request.args.get("q", "").strip()
    try:
        data = call_cpp("benchmark", keywords)
        return jsonify(data)
    except RuntimeError as e:
        return error_response(e)


# ===== Quản lý file corpus và thư mục upload =====
DATA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")
DATA_FILE = os.path.join(DATA_DIR, "document.txt")
UPLOADS_DIR = os.path.join(DATA_DIR, "uploads")
META_FILE = os.path.join(DATA_DIR, "files_meta.json")

os.makedirs(UPLOADS_DIR, exist_ok=True)


def load_meta():
    if os.path.exists(META_FILE):
        try:
            with open(META_FILE, "r", encoding="utf-8") as f:
                return json.load(f)
        except Exception:
            pass
    return []


def save_meta(files_list):
    with open(META_FILE, "w", encoding="utf-8") as f:
        json.dump(files_list, f, ensure_ascii=False, indent=2)


def rebuild_corpus_from_uploads():
    """Gộp tất cả file trong uploads/ vào document.txt theo danh sách meta."""
    meta = load_meta()
    all_lines = []
    updated_meta = []

    for item in meta:
        fname = item["filename"]
        fpath = os.path.join(UPLOADS_DIR, fname)
        if os.path.exists(fpath):
            with open(fpath, "r", encoding="utf-8-sig", errors="replace") as f:
                file_lines = [l.strip() for l in f if l.strip()]
            all_lines.extend(file_lines)
            updated_meta.append({"filename": fname, "docCount": len(file_lines)})

    with open(DATA_FILE, "w", encoding="utf-8") as f:
        for line in all_lines:
            f.write(line + "\n")

    save_meta(updated_meta)
    return updated_meta


DEFAULT_SAMPLE_TEXT = """AVL tree is a self-balancing binary search tree
Hash table provides average constant time search
Insertion in AVL tree requires rotation to maintain balance
Balance factor of every node in AVL tree must be -1 0 or 1
AVL tree height is always O(log n) due to strict balancing
Binary search tree without balancing can degrade to O(n)
Red black tree is another type of self-balancing tree
Rotation operations include left rotation and right rotation
Hash collision can be resolved using chaining or open addressing
Search engine uses inverted index to map keyword to documents
"""


def init_uploads():
    """Khởi tạo danh sách file ban đầu nếu chưa có meta."""
    meta = load_meta()
    if not meta:
        sample_name = "sample_docs.txt"
        sample_path = os.path.join(UPLOADS_DIR, sample_name)
        if not os.path.exists(sample_path):
            lines = [l.strip() for l in DEFAULT_SAMPLE_TEXT.splitlines() if l.strip()]
            with open(sample_path, "w", encoding="utf-8") as f:
                for l in lines:
                    f.write(l + "\n")
        save_meta([{"filename": sample_name, "docCount": 10}])
        rebuild_corpus_from_uploads()


init_uploads()

ABBR_SET = {"mr", "mrs", "ms", "dr", "prof", "sr", "jr", "vs", "etc", "e.g", "i.e", "u.s", "u.k"}


def split_sentences(text: str):
    """Tách đoạn văn thành các câu riêng biệt, đồng bộ với SentenceUtils.cpp."""
    text = text.lstrip("\ufeff").strip()
    if not text:
        return []
    sentences = []
    current = []
    chars = list(text)
    n = len(chars)
    i = 0
    while i < n:
        c = chars[i]
        current.append(c)
        if c in ".!?":
            if c == "." and i > 0 and i + 1 < n and chars[i - 1].isdigit() and chars[i + 1].isdigit():
                i += 1
                continue
            if c == ".":
                word = "".join(current[:-1]).split()
                if word and word[-1].lower() in ABBR_SET:
                    i += 1
                    continue
            j = i + 1
            while j < n and chars[j] in ".!?":
                current.append(chars[j])
                j += 1
            i = j - 1
            s = "".join(current).strip()
            if any(ch.isalnum() for ch in s):
                sentences.append(s)
            current = []
        i += 1
    rem = "".join(current).strip()
    if any(ch.isalnum() for ch in rem):
        sentences.append(rem)
    return sentences


def normalize_sentence(s: str) -> str:
    """Đảm bảo sau câu thêm vào luôn là một dấu kết câu (. ! ?) để sang câu mới."""
    s = s.strip()
    if s and not s.endswith((".", "!", "?")):
        s += "."
    return s


@app.route("/api/stats")
def stats():
    try:
        return jsonify(call_cpp("stats"))
    except RuntimeError as e:
        return error_response(e)


@app.route("/api/documents/files")
def list_files():
    try:
        return jsonify({"files": load_meta()})
    except Exception as e:
        return error_response(e)


@app.route("/api/documents/paragraphs")
def list_paragraphs():
    """Lấy danh sách các đoạn trong bài và từng câu trong đoạn để người dùng chọn vị trí chèn."""
    meta = load_meta()
    paragraphs = []
    global_doc_id = 0

    for item in meta:
        fname = item["filename"]
        fpath = os.path.join(UPLOADS_DIR, fname)
        if not os.path.exists(fpath):
            continue
        with open(fpath, "r", encoding="utf-8-sig", errors="replace") as f:
            lines = [l.strip() for l in f if l.strip()]
        for line_idx, line in enumerate(lines):
            sents = split_sentences(line)
            if not sents:
                sents = [line]
            paragraphs.append({
                "docId": global_doc_id,
                "filename": fname,
                "lineIndex": line_idx,
                "text": line,
                "sentences": sents
            })
            global_doc_id += 1

    return jsonify({
        "paragraphs": paragraphs,
        "totalDocs": global_doc_id,
        "files": [m["filename"] for m in meta]
    })


@app.route("/api/documents/insert_sentence", methods=["POST"])
def insert_sentence():
    """Chèn một câu hoàn chỉnh mới vào một vị trí cụ thể (đoạn và câu) thẳng vào file."""
    body = request.get_json(silent=True) or {}
    raw_sentence = (body.get("sentence") or "").strip().replace("\r", " ").replace("\n", " ")
    if not raw_sentence:
        return error_response(ValueError("Nội dung câu không được để trống"), 400)

    # Đảm bảo sau câu thêm vào luôn là một dấu kết câu để sang câu mới
    sentence = normalize_sentence(raw_sentence)

    doc_id = body.get("docId")
    position = body.get("position", "after")  # "before", "after", "end", "new_paragraph"
    try:
        sentence_index = int(body.get("sentenceIndex", -1))
    except (TypeError, ValueError):
        sentence_index = -1

    meta = load_meta()
    if not meta:
        init_uploads()
        meta = load_meta()

    try:
        # Trường hợp thêm thành đoạn mới ở cuối bài
        if position == "new_paragraph" or doc_id is None or str(doc_id) == "new_paragraph" or int(doc_id) < 0:
            target_file = body.get("targetFile") or (meta[-1]["filename"] if meta else "sample_docs.txt")
            fpath = os.path.join(UPLOADS_DIR, target_file)
            with open(fpath, "a", encoding="utf-8") as f:
                f.write(sentence + "\n")
            doc_label = "mới ở cuối bài"
            file_name = target_file
        else:
            doc_id = int(doc_id)
            target_item = None
            target_line_idx = -1
            current_offset = 0

            for item in meta:
                fname = item["filename"]
                fpath = os.path.join(UPLOADS_DIR, fname)
                if not os.path.exists(fpath):
                    continue
                with open(fpath, "r", encoding="utf-8-sig", errors="replace") as f:
                    file_lines = [l.strip() for l in f if l.strip()]
                count = len(file_lines)
                if current_offset <= doc_id < current_offset + count:
                    target_item = item
                    target_line_idx = doc_id - current_offset
                    break
                current_offset += count

            if not target_item or target_line_idx < 0:
                return error_response(ValueError(f"Không tìm thấy đoạn docId={doc_id}"), 404)

            file_name = target_item["filename"]
            fpath = os.path.join(UPLOADS_DIR, file_name)
            with open(fpath, "r", encoding="utf-8-sig", errors="replace") as f:
                file_lines = [l.strip() for l in f if l.strip()]

            orig_line = file_lines[target_line_idx]
            sents = split_sentences(orig_line)
            if not sents:
                sents = [orig_line]
            # Đảm bảo mỗi câu trước đó đều có dấu kết câu chuẩn
            sents = [normalize_sentence(s) for s in sents]

            if position == "before":
                idx = max(0, min(sentence_index if sentence_index >= 0 else 0, len(sents)))
                sents.insert(idx, sentence)
            elif position == "after":
                idx = max(0, min((sentence_index + 1) if sentence_index >= 0 else len(sents), len(sents)))
                sents.insert(idx, sentence)
            else:  # "end"
                sents.append(sentence)

            # Ghép lại thành dòng hoàn chỉnh (mỗi câu phân cách bằng 1 dấu cách)
            updated_line = " ".join(sents)
            file_lines[target_line_idx] = updated_line

            with open(fpath, "w", encoding="utf-8") as f:
                for l in file_lines:
                    f.write(l + "\n")

            doc_label = f"Đoạn {doc_id + 1}"

        updated_files = rebuild_corpus_from_uploads()
        stats = call_cpp("stats")
        return jsonify({
            "success": True,
            "added": True,
            "message": f"Đã thêm câu vào {doc_label} trong file '{file_name}' thành công ✓",
            "insertedSentence": sentence,
            "docId": doc_id,
            "files": updated_files,
            "stats": stats
        })
    except (RuntimeError, OSError) as e:
        return error_response(e)


@app.route("/api/documents/add", methods=["POST"])
def add_document():
    body = request.get_json(silent=True) or {}
    if "sentence" in body or "docId" in body or "position" in body:
        return insert_sentence()

    # Tương thích ngược: nếu chỉ gửi text, thêm thành đoạn mới ở cuối bài
    text = (body.get("text") or "").strip()
    if not text:
        return error_response(ValueError("Thiếu nội dung document"), 400)
    body["sentence"] = text
    body["position"] = "new_paragraph"
    return insert_sentence()


@app.route("/api/documents/upload", methods=["POST"])
def upload_documents():
    if "file" not in request.files or request.files["file"].filename == "":
        return error_response(ValueError("Chua chon file"), 400)
    file = request.files["file"]
    upload_mode = request.form.get("mode", "append")
    safe_filename = os.path.basename(file.filename).strip() or "uploaded_document.txt"

    try:
        content = file.read().decode("utf-8-sig", errors="replace")
        lines = [l.strip() for l in content.splitlines() if l.strip()]
        if not lines:
            return error_response(ValueError("File rong hoac khong co dong hop le"), 400)

        if upload_mode == "replace":
            for f in os.listdir(UPLOADS_DIR):
                fpath = os.path.join(UPLOADS_DIR, f)
                if os.path.isfile(fpath):
                    try:
                        os.remove(fpath)
                    except OSError:
                        pass
            meta = []
        else:
            meta = load_meta()
            meta = [m for m in meta if m["filename"] != safe_filename]

        target_path = os.path.join(UPLOADS_DIR, safe_filename)
        with open(target_path, "w", encoding="utf-8") as f:
            for line in lines:
                f.write(line + "\n")

        meta.append({"filename": safe_filename, "docCount": len(lines)})
        save_meta(meta)
        updated_files = rebuild_corpus_from_uploads()

        return jsonify({
            "added": len(lines),
            "filename": safe_filename,
            "mode": upload_mode,
            "files": updated_files,
            "stats": call_cpp("stats")
        })
    except (RuntimeError, OSError, UnicodeDecodeError) as e:
        return error_response(e)


@app.route("/api/documents/remove", methods=["POST"])
def remove_document_file():
    body = request.get_json(silent=True) or {}
    filename = (body.get("filename") or "").strip()
    if not filename:
        return error_response(ValueError("Thieu ten file can xoa"), 400)

    safe_filename = os.path.basename(filename)
    fpath = os.path.join(UPLOADS_DIR, safe_filename)
    if os.path.exists(fpath):
        try:
            os.remove(fpath)
        except OSError as e:
            return error_response(e)

    meta = load_meta()
    meta = [m for m in meta if m["filename"] != safe_filename]
    save_meta(meta)
    updated_files = rebuild_corpus_from_uploads()

    return jsonify({
        "removed": safe_filename,
        "files": updated_files,
        "stats": call_cpp("stats")
    })


@app.route("/api/documents/clear", methods=["POST"])
def clear_all_documents():
    try:
        for f in os.listdir(UPLOADS_DIR):
            fpath = os.path.join(UPLOADS_DIR, f)
            if os.path.isfile(fpath):
                try:
                    os.remove(fpath)
                except OSError:
                    pass

        save_meta([])
        with open(DATA_FILE, "w", encoding="utf-8") as f:
            pass  # Làm trống file

        return jsonify({
            "cleared": True,
            "files": [],
            "stats": call_cpp("stats")
        })
    except (RuntimeError, OSError) as e:
        return error_response(e)


@app.route("/api/documents/reset_sample", methods=["POST"])
def reset_sample_documents():
    try:
        sample_name = "sample_docs.txt"
        sample_path = os.path.join(UPLOADS_DIR, sample_name)
        lines = [l.strip() for l in DEFAULT_SAMPLE_TEXT.splitlines() if l.strip()]
        with open(sample_path, "w", encoding="utf-8") as f:
            for l in lines:
                f.write(l + "\n")
        save_meta([{"filename": sample_name, "docCount": len(lines)}])
        updated_files = rebuild_corpus_from_uploads()
        return jsonify({
            "reset": True,
            "files": updated_files,
            "stats": call_cpp("stats")
        })
    except (RuntimeError, OSError) as e:
        return error_response(e)


if __name__ == "__main__":
    port = int(os.environ.get("PORT", 5000))
    app.run(host="0.0.0.0", port=port, debug=False)
