(loopwhile 1
    (progn
        (define ppm (get-ppm)) ; This will make ppm show up in the variable table
        (set-duty ppm)
        (yield 20000)
))
