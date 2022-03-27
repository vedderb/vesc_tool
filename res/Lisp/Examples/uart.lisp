(uart-start 115200)

(define uart-buf (array-create type-byte 100))

(defun read-thd ()
    (loopwhile t
        (progn
            (uart-read-until uart-buf 30 0 13) ; 13 is the return key
            (print uart-buf)
)))

(spawn 150 read-thd) ; Run reader in its own thread

(loopwhile t
    (progn
        (uart-write "Test\r\n")
        (sleep 1.0)
))
