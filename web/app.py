import subprocess
import json
import os
from flask import Flask, jsonify, request, render_template

app = Flask(__name__)

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
        return jsonify({"docIds": data})
    except RuntimeError as e:
        return error_response(e)


@app.route("/api/search/ranked")
def search_ranked():
    keyword = request.args.get("q", "").strip()
    if not keyword:
        return error_response(ValueError("Thieu tham so q"), 400)
    try:
        data = call_cpp("ranked", keyword)
        return jsonify({"results": data})
    except RuntimeError as e:
        return error_response(e)


@app.route("/api/search/and")
def search_and():
    keywords = request.args.get("q", "").strip()  # dạng "avl,rotation"
    if not keywords:
        return error_response(ValueError("Thieu tham so q"), 400)
    try:
        data = call_cpp("and", keywords)
        return jsonify({"docIds": data})
    except RuntimeError as e:
        return error_response(e)


@app.route("/api/search/or")
def search_or():
    keywords = request.args.get("q", "").strip()
    if not keywords:
        return error_response(ValueError("Thieu tham so q"), 400)
    try:
        data = call_cpp("or", keywords)
        return jsonify({"docIds": data})
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
            with open(fpath, "r", encoding="utf-8", errors="replace") as f:
                file_lines = [l.strip() for l in f if l.strip()]
            all_lines.extend(file_lines)
            updated_meta.append({"filename": fname, "docCount": len(file_lines)})

    with open(DATA_FILE, "w", encoding="utf-8") as f:
        for line in all_lines:
            f.write(line + "\n")

    save_meta(updated_meta)
    return updated_meta


def init_uploads():
    """Khởi tạo danh sách file ban đầu nếu chưa có meta."""
    if not os.path.exists(META_FILE):
        initial_files = []
        if os.path.exists(DATA_FILE):
            with open(DATA_FILE, "r", encoding="utf-8", errors="replace") as f:
                lines = [l.strip() for l in f if l.strip()]
            if lines:
                seed_name = "document.txt"
                seed_path = os.path.join(UPLOADS_DIR, seed_name)
                with open(seed_path, "w", encoding="utf-8") as f:
                    for l in lines:
                        f.write(l + "\n")
                initial_files.append({"filename": seed_name, "docCount": len(lines)})
        save_meta(initial_files)


init_uploads()


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


@app.route("/api/documents/add", methods=["POST"])
def add_document():
    body = request.get_json(silent=True) or {}
    text = (body.get("text") or "").strip()
    if not text:
        return error_response(ValueError("Thieu noi dung document"), 400)
    if "\n" in text:
        return error_response(ValueError("Document khong duoc chua xuong dong"), 400)
    try:
        manual_name = "manual_notes.txt"
        manual_path = os.path.join(UPLOADS_DIR, manual_name)
        with open(manual_path, "a", encoding="utf-8") as f:
            f.write(text + "\n")

        meta = load_meta()
        found = False
        for m in meta:
            if m["filename"] == manual_name:
                found = True
                break
        if not found:
            meta.append({"filename": manual_name, "docCount": 1})
            save_meta(meta)

        updated_files = rebuild_corpus_from_uploads()
        return jsonify({"added": True, "files": updated_files, "stats": call_cpp("stats")})
    except (RuntimeError, OSError) as e:
        return error_response(e)


@app.route("/api/documents/upload", methods=["POST"])
def upload_documents():
    if "file" not in request.files or request.files["file"].filename == "":
        return error_response(ValueError("Chua chon file"), 400)
    file = request.files["file"]
    upload_mode = request.form.get("mode", "append")
    safe_filename = os.path.basename(file.filename).strip() or "uploaded_document.txt"

    try:
        content = file.read().decode("utf-8", errors="replace")
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


if __name__ == "__main__":
    port = int(os.environ.get("PORT", 5000))
    app.run(host="0.0.0.0", port=port, debug=False)