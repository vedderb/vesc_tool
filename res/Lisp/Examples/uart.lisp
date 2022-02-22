(define uart-buf (array-create type-byte 100))

; As uart-read returns right away when there is nothing to read we create
; wrappers that try to read bytes until the desired amount of bytes is
; received. They use yield to give other threads a change to run.

; Read n bytes into buffer at ofs.
(define read-bytes (lambda (buffer n ofs)
    (let 
        (
            (rd (uart-read buffer n ofs))
        )
        (if (num-eq rd n)
            (bufset-u8 buffer 0 (+ ofs rd)) ; Newline at the end
            (progn (yield 4000) (read-bytes buffer (- n rd) (+ ofs rd)))
        )
    )
))

; Read at most n bytes into buffer at ofs. Stop reading and return at characted end.
(define read-until (lambda (buffer n ofs end)
    (let
        (
            (rd (uart-read buffer n ofs end))
        )
        (if (or (num-eq rd n) (num-eq (bufget-u8 buffer (+ ofs (- rd 1))) end))
            (bufset-u8 buffer 0 (+ ofs rd))
            (progn (yield 10000) (read-until buffer (- n rd) (+ ofs rd) end))
        )
    )
))

(define read-thd (lambda ()
    (progn
        (read-until uart-buf 30 0 13) ; 13 is the return key
;        (read-bytes uart-buf 6 0)
        (print uart-buf)
        (read-thd)
)))

(define f (lambda ()
    (progn
        (uart-write "Test")
        (yield 1000000)
        (f)
)))

(uart-start 115200)
(yield 10000)
(spawn 100 read-thd)

(f)
