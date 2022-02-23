(define uart-buf (array-create type-byte 100))

(defun read-thd ()
    (progn
        (uart-read-until uart-buf 30 0 13) ; 13 is the return key
;        (uart-read-bytes uart-buf 6 0)
        (print uart-buf)
        (read-thd)
))

(defun f ()
    (progn
        (uart-write "Test")
        (yield 1000000)
        (f)
))

(uart-start 115200)
(spawn 100 read-thd)
(f)
