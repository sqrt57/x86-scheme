(if #t 5 (car 5))
(define if1
  (lambda (condition then else)
    (if condition then else)))
(define if2
  (macro (condition then else)
    (if (eval condition) then else)))
(if2 #t 5 (car 5))
(if2 #f 5 (car 5))

