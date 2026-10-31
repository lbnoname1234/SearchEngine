import subprocess
import json
import os
from flask import Flask, jsonify, request, render_template

app = Flask(__name__)

# Đường dẫn tới file .exe đã build từ main_cli.cpp
# Đặt search_cli.exe cùng thư mục với app.py, hoặc sửa lại đường dẫn tuyệt đối
CLI_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "search_cli.exe")


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


# ===== Đường dẫn tới data/document.txt, dùng cho add/upload =====
DATA_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", "document.txt")


@app.route("/api/stats")
def stats():
    try:
        return jsonify(call_cpp("stats"))
    except RuntimeError as e:
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
        with open(DATA_FILE, "a", encoding="utf-8") as f:
            f.write(text + "\n")
        return jsonify({"added": True, "stats": call_cpp("stats")})
    except (RuntimeError, OSError) as e:
        return error_response(e)


@app.route("/api/documents/upload", methods=["POST"])
def upload_documents():
    if "file" not in request.files or request.files["file"].filename == "":
        return error_response(ValueError("Chua chon file"), 400)
    file = request.files["file"]
    # mode = "replace" -> xoa sach du lieu cu truoc khi ghi file moi vao
    # mode = "append" (mac dinh) -> giu nguyen du lieu cu, chi them vao cuoi
    upload_mode = request.form.get("mode", "append")
    try:
        content = file.read().decode("utf-8", errors="replace")
        lines = [l.strip() for l in content.splitlines() if l.strip()]
        if not lines:
            return error_response(ValueError("File rong hoac khong co dong hop le"), 400)

        write_mode = "w" if upload_mode == "replace" else "a"
        with open(DATA_FILE, write_mode, encoding="utf-8") as f:
            for line in lines:
                f.write(line + "\n")

        return jsonify({"added": len(lines), "mode": upload_mode, "stats": call_cpp("stats")})
    except (RuntimeError, OSError, UnicodeDecodeError) as e:
        return error_response(e)


if __name__ == "__main__":
    app.run(debug=True, port=5000)