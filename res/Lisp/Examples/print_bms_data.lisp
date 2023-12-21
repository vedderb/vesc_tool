(loopwhile 1
   (progn
     (define v-cells (map (lambda (c)(get-bms-val 'bms-v-cell c)) (range 0 (get-bms-val 'bms-cell-num))))
     (print (list "V Tot  : " (get-bms-val 'bms-v-tot)))
     (print (list "V Cells: " v-cells))
     (print (list "foldl  : " (foldl + 0 v-cells)))
     (print (list "ADCs   : " (map (lambda (c)(get-adc c)) (iota 2))))
     (print " ")
     (sleep 1)
))
