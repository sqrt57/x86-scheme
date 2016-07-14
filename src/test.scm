((lambda (x . y) x) 5)
((lambda (x . y) y) 5)
((lambda (x . y) y) 5 6 7)
((lambda (x y z) (- (+ x y) z)) 100 20 5)
((lambda (x) ((lambda (x) x) 5)) 8)
((lambda (x) ((lambda (y) x) 5)) 8)

