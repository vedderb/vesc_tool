(define rate 2) ; Update rate in hz

(let ((f (lambda ()
   (progn
     (define v-cells (map (lambda (c)(get-bms-val "v_cell" c)) (range 0 (- (get-bms-val "cell_num") 1))))
     (print (list "V Tot  : " (get-bms-val "v_tot")))
     (print (list "V Cells: " v-cells))
     (print (list "foldl  : " (foldl + 0 v-cells)))
     (print (list "ADCs   : " (map (lambda (c)(get-adc c)) (iota 1))))
     (print " ")
     (yield (/ 1000000.0 rate))
     (f)
)))) (f))

