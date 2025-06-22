(loopwhile t {
        (def v-cells (map (fn (c)(get-bms-val 'bms-v-cell c)) (range 0 (get-bms-val 'bms-cell-num))))
        (print (list "V Tot  : " (get-bms-val 'bms-v-tot)))
        (print (list "V Cells: " v-cells))
        (print (list "foldl  : " (foldl + 0 v-cells)))
        (print (list "ADCs   : " (map (fn (c)(get-adc c)) (range 2))))
        (print " ")
        (sleep 1)
})
