# Project Q&A (song ngữ) — Multi-thread File Indexer

Câu hỏi hay bị hỏi khi review/phỏng vấn, kèm câu trả lời viết dễ hiểu, bám theo code thật.
Bilingual interview/review questions with plain-language answers grounded in the actual code.

> Ghi chú đọc: mỗi mục có phần **Trả lời** (giải thích thường) và **Nói ngắn khi phỏng vấn** (câu chốt để trả lời nhanh).

---

## 1. Kiến trúc tổng thể / Overall architecture

**VN:** Chương trình index thư mục hoạt động theo luồng nào?
**EN:** How does data flow through the indexer?

**Trả lời:**
Có 2 vai: **người bỏ việc vào (producer)** và **người làm việc (worker/consumer)**.
- Thread chính chạy `index_directory`: nó **đi hết cây thư mục**, gặp file nào thì **bỏ một "việc index file đó" vào hàng đợi** của thread pool.
- Các **worker thread** lấy việc ra, **đọc file → tách chữ (tokenize) → ghi vào chỉ mục (inverted index) dùng chung**.
- Thread chính **đứng chờ tất cả việc xong** (gọi `f.get()` trên từng `future`). Nếu 1 worker lỗi, lỗi đó cũng được **đẩy về (propagate)** cho thread chính biết.

**Nói ngắn khi phỏng vấn:** "Một thread duyệt thư mục và bỏ mỗi file thành 1 task; nhiều worker xử lý song song rồi ghi vào chỉ mục chung; thread chính chờ hết task và nhận lỗi nếu có."

**EN:** One thread walks the tree and queues one task per file; worker threads process files in parallel and write into a shared index; the main thread waits for all tasks and receives any worker error.

---

## 2. Thread pool

**VN:** Vì sao dùng `packaged_task` + `future`? Khi tắt pool, làm sao join thread an toàn?
**EN:** Why `packaged_task` + `future`? How is shutdown/join made safe?

**Trả lời:**
- `future` giống **"phiếu nhận hàng"**: bỏ việc vào, nhận về một phiếu; sau này gọi `get()` để **lấy kết quả** hoặc **nhận lỗi** nếu việc đó ném exception. `packaged_task` là thứ gói việc lại để gắn với phiếu đó.
- Lúc tắt (destructor), phải chắc chắn **thread dừng trước, hàng đợi bị xóa sau** — nếu xóa hàng đợi trong khi thread còn dùng thì crash (use-after-free). Ở đây tận dụng **thứ tự khai báo biến**: hàng đợi `m_tasks` khai báo **trước** nên **bị hủy sau cùng**; đám thread `m_workers` khai báo **sau** nên **bị hủy trước**. Destructor báo "hết việc, dừng đi" (`shutdown()`), rồi các thread mới bị hủy → chúng join an toàn vì **hàng đợi vẫn còn sống**.

**Nói ngắn khi phỏng vấn:** "`future` để lấy kết quả và lỗi. Khi tắt, nhờ thứ tự khai báo member, thread bị join trước khi hàng đợi bị hủy, nên không use-after-free."

**EN:** `future` carries the result or the error; on shutdown, member declaration order guarantees threads are joined while the task queue is still alive, so there is no use-after-free.

**VN:** Nếu máy báo số nhân CPU = 0 (`hardware_concurrency()` trả 0) thì sao?
**EN:** What if `hardware_concurrency()` returns 0?

**Trả lời:** Hàm này **đôi khi trả 0** (không xác định được số nhân). Pool tự **ép về 1** để luôn có ít nhất 1 worker; `main` cũng ép về 1 để **con số in ra khớp thực tế**.

**EN:** That function can return 0; the pool clamps it to 1, and `main` clamps too so the printed thread count matches reality.

---

## 3. Hàng đợi có chờ (`ts_queue`) / Blocking queue

**VN:** "Lost-wakeup" là gì và tránh thế nào? Vì sao worker thoát được khi shutdown?
**EN:** What is lost-wakeup, how is it avoided, and how do workers exit on shutdown?

**Trả lời:**
- **Lost-wakeup** = tín hiệu "có việc rồi, dậy đi" bị **lọt mất** đúng lúc thread chuẩn bị ngủ, khiến nó **ngủ mãi**. Tránh bằng cách: cả lúc **thêm việc** (`push`) lẫn lúc **báo dừng** (`shutdown`) đều **đổi trạng thái trong khi đang giữ khóa (mutex)** rồi mới đánh thức. Nhờ vậy thread không thể "ngủ nhầm khe".
- `wait_pop` chỉ ngủ khi **hàng rỗng VÀ chưa dừng**. Khi shutdown: nếu còn việc thì **làm nốt cho hết** rồi mới thoát; nếu hết việc thì trả `false` để vòng lặp worker kết thúc.

**Nói ngắn khi phỏng vấn:** "Đổi trạng thái dưới mutex rồi mới notify nên không mất tín hiệu. Shutdown làm nốt việc còn lại rồi cho worker thoát."

**EN:** State is changed under the lock before notifying, so no wakeup is lost; on shutdown remaining tasks are drained, then workers exit.

---

## 4. Hàng đợi hai khóa (`two_lock_queue`) / Two-lock queue

**VN:** Nó là gì, khác `ts_queue` chỗ nào, sao không dùng cho pool?
**EN:** What is it, how does it differ, and why not use it for the pool?

**Trả lời:**
- Đây là hàng đợi kiểu **Michael & Scott**: **đầu (head) và đuôi (tail) có khóa riêng**, nên **một người thêm và một người lấy chạy song song được** (nhanh hơn khi tranh chấp so với 1 khóa chung).
- Chỗ duy nhất hai bên đụng nhau là con trỏ `next` của node ranh giới → để nó là **biến atomic** với thứ tự **release/acquire**, tức là node mới được "công bố" đầy đủ trước khi bên lấy nhìn thấy (không dính data race).
- Có **1 node giả (dummy)** để đầu và đuôi không đụng nhau khi rỗng.
- Nó **chỉ có `try_pop` (không chờ)**. Làm kiểu "có chờ" trên hàng hai khóa **rất khó đúng**, nên pool xài `ts_queue` (loại có chờ) thay vì cái này. Hiện `two_lock_queue` **chỉ dùng trong test**.

**Nói ngắn khi phỏng vấn:** "Head/tail khóa riêng cho song song; `next` là atomic release/acquire. Nó không hỗ trợ chờ nên pool dùng `ts_queue`."

**EN:** Separate head/tail locks allow concurrency; the boundary `next` is an atomic (release/acquire). It only supports non-blocking `try_pop`, so the pool uses the blocking `ts_queue`.

---

## 5. Chỉ mục chia mảnh (sharded inverted index)

**VN:** Vì sao chia "shard"? Sao chọn 16? `for_each` có an toàn khi nhiều thread không?
**EN:** Why shard? Why 16? Is `for_each` concurrency-safe?

**Trả lời:**
- **Inverted index** = bảng tra ngược "**chữ → những file chứa chữ đó**".
- **Shard** = **chia bảng thành 16 ngăn nhỏ, mỗi ngăn 1 khóa riêng**. Chữ nào rơi vào ngăn nào là do `hash(chữ) % 16`. Nhờ vậy các thread ghi **chữ khác nhau thường vào ngăn khác nhau → không phải tranh nhau 1 khóa chung** (đây là mẹo chính giúp chạy nhanh khi nhiều thread).
- Cùng 1 file thêm nhiều lần cho 1 chữ **không bị trùng** vì mỗi chữ giữ danh sách file bằng `set`.
- `for_each` (duyệt toàn bộ để ghi ra SQLite) khóa **từng ngăn một**, nhưng **không được chạy khi vẫn còn thread đang ghi** — chỉ gọi **sau khi index xong**.

**Nói ngắn khi phỏng vấn:** "Chia 16 ngăn có khóa riêng để giảm tranh chấp; `for_each` chỉ dùng sau khi index xong, không an toàn nếu còn writer."

**EN:** 16 independently locked buckets reduce contention; `for_each` is safe only after indexing finishes.

---

## 6. Lưu ra SQLite / Persistence

**VN:** Giải thích cấu trúc bảng và vì sao "chuẩn hóa"? Vì sao ghi trong 1 transaction?
**EN:** Explain the schema/normalization and the single-transaction write.

**Trả lời:**
- 3 bảng: `terms` (mỗi **chữ** lưu 1 lần), `docs` (mỗi **đường dẫn file** lưu 1 lần), `postings` (**cặp số**: chữ số mấy nằm ở file số mấy).
- **Chuẩn hóa (normalization)** = **không lặp lại chuỗi**: thay vì lưu lại cả chữ và đường dẫn dài dòng trong mỗi dòng, ta lưu **số id** cho gọn. `postings` chỉ là cặp số nguyên nên rất nhẹ.
- Khóa chính ghép `(term_id, doc_id)` cũng là cách sắp xếp dữ liệu trên disk (**clustered**) → **tra "file nào chứa chữ X" chạy rất nhanh**, và **cặp trùng bị loại tự động**.
- Ghi **tất cả trong 1 transaction** thay vì lưu từng dòng: mỗi lần "chốt" ghi disk (fsync) rất tốn; chốt hàng nghìn lần sẽ **chậm gấp nhiều lần**. Dùng WAL + `synchronous=NORMAL` cho vừa nhanh vừa đủ an toàn.
- Có cache `đường dẫn → doc_id` để **mỗi file chỉ tạo đúng 1 dòng** trong `docs`.

**Nói ngắn khi phỏng vấn:** "3 bảng chuẩn hóa để không lặp chuỗi; postings là cặp số với khóa ghép làm index; ghi 1 transaction để tránh fsync từng dòng."

**EN:** Three normalized tables avoid repeating strings; postings are integer pairs with a composite key as the index; one transaction avoids per-row fsync.

**VN:** Vì sao dùng `sqlite3_close_v2` chứ không `sqlite3_close`?
**EN:** Why `sqlite3_close_v2` instead of `sqlite3_close`?

**Trả lời:** Khi có lỗi giữa chừng, các câu lệnh đã chuẩn bị (**prepared statement**) chưa được dọn (`finalize`). `sqlite3_close` gặp trường hợp này sẽ **từ chối đóng** (trả `SQLITE_BUSY`) và **để rò rỉ** kết nối. `sqlite3_close_v2` **đóng an toàn**, tự dọn khi câu lệnh cuối được giải phóng.

**EN:** On the error path some prepared statements are still open; `sqlite3_close` refuses to close (returns `SQLITE_BUSY`) and leaks the handle, while `sqlite3_close_v2` closes safely.

---

## 7. Chuẩn hóa chữ nhất quán / Consistent tokenization

**VN:** Làm sao đảm bảo lúc index và lúc tìm kiếm xử lý chữ giống nhau?
**EN:** How do indexing and querying stay consistent?

**Trả lời:** Cả hai đều theo **cùng một quy tắc**: **đổi thành chữ thường + bỏ ký tự không phải chữ/số**. Quy tắc này gói trong 1 hàm chung `normalize_word`. Nhờ dùng chung, `"Hello,"` và `"hello"` luôn thành cùng 1 chữ, và **lúc tìm không bao giờ lệch với lúc index**.

**EN:** Both lowercase and strip non-alphanumerics via the shared `normalize_word`, so search can never disagree with indexing.

---

## 8. Trường hợp biên & độ bền / Edge cases & robustness

**VN:** File nhị phân, file rỗng, file không có quyền đọc, lỗi khi đang index thì sao?
**EN:** Binary/empty files, permission-denied files, errors during indexing?

**Trả lời:**
- **File nhị phân/rỗng:** đọc theo byte, chỉ sinh ra vài "chữ rác" vô hại, không crash.
- **Không có quyền đọc:** khi duyệt thư mục dùng `skip_permission_denied` + `error_code` để **bỏ qua** entry lỗi thay vì ném lỗi.
- **Mở file lỗi:** hàm `index_file` **return sớm**, không làm gì.
- **Lỗi trong worker:** được **đẩy về (propagate)** qua `f.get()`; `main` bọc bước index trong `try/catch` để **in lỗi gọn gàng** thay vì để chương trình chết đột ngột (`std::terminate`).
- **Hạn chế đã biết:** nếu 1 `future` ném lỗi thì các `future` còn lại **không được chờ tiếp** (dừng ở lỗi đầu tiên).

**EN:** Binary-safe reads, permission-denied entries skipped, open failures return early, worker errors propagate through `get()` and are caught in `main`. Known limitation: on the first failing future, the rest aren't awaited.

---

## 9. Kiểm thử / Testing

**VN:** Chiến lược test ra sao? Kiểm tra tính đúng khi chạy đa luồng thế nào?
**EN:** Testing strategy? How is concurrency correctness checked?

**Trả lời:**
- **Không dùng framework**, tự viết harness nhỏ; chương trình trả về mã 0 nếu tất cả pass.
- Test **đa luồng thật**: 1 người thêm + 1 người lấy 100.000 phần tử cho `two_lock_queue` (kiểm tổng đúng), 8 thread cùng ghi `inverted_index` (kiểm số chữ và số file/chữ), pool tính 100 phép, và test **lưu ra SQLite rồi đọc lại** cho `file_indexer`.
- CMake có bật **ThreadSanitizer** (`ENABLE_TSAN`) - công cụ **bắt lỗi tranh chấp dữ liệu (data race)** khi chạy test.

**EN:** A tiny framework-free harness; real multithreaded tests plus a file→SQLite→query roundtrip; a ThreadSanitizer build option catches data races.

---

## 10. Câu hỏi mở rộng / Follow-up design questions

**Nếu chỉ mục quá lớn không vừa RAM? / What if the index doesn't fit in RAM?**
- **VN:** Ghi ra SQLite theo từng đợt (batch), hoặc trộn (merge) các phần postings từ disk thay vì giữ hết trong bộ nhớ.
- **EN:** Flush to SQLite in batches, or external-merge the postings from disk instead of keeping everything in memory.

**Còn nghẽn (contention) ở đâu? / Where is the remaining contention?**
- **VN:** Nhiều thread ghi cùng 1 chữ "nóng" vẫn dồn vào 1 ngăn; có thể tăng số ngăn hoặc dùng cấu trúc map hỗ trợ đồng thời (concurrent map).
- **EN:** Many threads writing the same hot word still hit one shard; increase the shard count or use a concurrent map.

**Index lại chỉ phần thay đổi (incremental)? / How to re-index only what changed?**
- **VN:** Hiện `save` ghi đè toàn bộ; cần lưu thời gian sửa (mtime) hoặc hash của file để chỉ cập nhật file đã đổi.
- **EN:** `save` currently overwrites everything; track each file's mtime or hash to update only changed files.

**Xếp hạng kết quả (tf-idf)? / How to rank results (tf-idf)?**
- **VN:** Cần lưu thêm số lần xuất hiện của chữ trong mỗi file; hiện `postings` chỉ lưu "có/không chứa".
- **EN:** Store the term frequency per file in `postings`; today it only records presence/absence.
