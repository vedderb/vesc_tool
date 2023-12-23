; Generate entry in CRC-table. Can be used to pre-generate a CRC-table for speed
; or to generate the index in place.
(defun crc-gentab (ind poly)
    (let (
            (helper (fn (bit crc)
                    (if (= bit 8)
                        crc
                        (helper
                            (+ bit 1)
                            (bitwise-and
                                (if (> (bitwise-and crc 0x8000) 0)
                                    (bitwise-xor (shl crc 1) poly)
                                    (shl crc 1)
                                )
                                0xFFFF
                        ))
            )))
        ) (helper 0 (shl ind 8))
))

; This gives the same CRC as crc16 in the VESC code
(def poly 0x11021)

(defun crc16-lbm (data)
    (let (
            (len (buflen data))
            (helper
                (fn (crc ind)
                    (if (= ind len)
                        crc
                        (helper
                            (bitwise-xor
                                (crc-gentab
                                    (bitwise-and (bitwise-xor (shr crc 8) (bufget-u8 data ind)) 0xFF)
                                    poly
                            ) (bitwise-and (shl crc 8) 0xFFFF))
                            (+ ind 1)
            ))))
        ) (helper 0 0)
))

(def crc-tab (array-create 512))
(looprange i 0 256 (bufset-u16 crc-tab (* i 2) (crc-gentab i poly)))

(defun crc16-precalc (data)
    (let (
            (len (buflen data))
            (helper
                (fn (crc ind)
                    (if (= ind len)
                        crc
                        (helper
                            (bitwise-xor
                                (bufget-u16
                                    crc-tab
                                    (* 2 (bitwise-and (bitwise-xor (shr crc 8) (bufget-u8 data ind)) 0xFF))
                            )(bitwise-and (shl crc 8) 0xFFFF))
                            (+ ind 1)
            ))))
        ) (helper 0 0)
))

; Speed test

(def start (systime))
(crc16-lbm "Hello World!")
(print (secs-since start))

(def start (systime))
(crc16-precalc "Hello World!")
(print (secs-since start))

; Builtin
(def start (systime))
(crc16 "Hello World!")
(print (secs-since start))

; Add CRC to end of string as hex
(defun str-crc-add (str)
    (str-merge str (str-from-n (crc16-precalc str) "%04x"))
)

; Check CRC at end of string
(defun str-crc-check (str)
    (let (
            (len (str-len str))
            (strx (str-part str 0 (- len 4)))
            (crc-rx (str-part str (- len 4)))
            (crc-calc (str-from-n (crc16-precalc strx) "%04x"))
        ) (eq crc-rx crc-calc)
))

; The following code can be used to print the generated CRC table as an array
; that can be pasted into the code

(defun print-crc-tab () {
        (print "(def crc-tab [")
        (var rows 32)
        (var cols 16)
        
        (looprange i 0 rows
            (print (apply str-merge
                    (map
                        (fn (x) (str-from-n (bufget-u8 crc-tab (+ x (* i cols))) "0x%02x "))
                        (range cols)
                    
                    )
        )))
        (print "])")
})
