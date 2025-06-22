; This is how to print a string in LispBM to the console below
(print "Hello World!")

; You can also print lisp types, such as the list (1 2 3 4 5)
(print (list 1 2 3 4 5))

; Here we print the index of a range loop
(looprange i 1 5
    (print i)
)

; This is how to print the input voltage. See
; https://github.com/vedderb/bldc/tree/master/lispBM#get-vin
(print (get-vin))

; The string functions can be used to format a string
; before printing it. See
; https://github.com/vedderb/bldc/tree/master/lispBM#string-manipulation
(print (str-from-n (get-vin) "Input Voltage: %.2f V"))

