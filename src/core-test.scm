(test-begin "core" 2)

(test-assert "addition" (= (+ 1 2) 3))

(define (tst)
  (define x 1)
  (begin
    (define y 7)
    (define x 3))
  (+ x y))
(test-assert "begin" (= (tst) 10))

(test-assert "eq" (eq? (quote a) (quote a)))

(test-eq "not-#t" (not #t) #f)
(test-eq "not-#f" (not #f) #t)
(test-eq "not-str" (not "abc") #f)

(test-assert "pair?-pair" (pair? (quote (a . b))))
(test-assert "pair?-empty-list" (not (pair? (quote ()))))
(test-assert "null?-empty-list" (null? (quote ())))
(test-assert "null?-symbol" (not (null? (quote a))))

(test-assert "or2-#t-#f" (or2 #t #f))
(test-assert "or2-#f-#t" (or2 #f #t))
(test-assert "or2-#t-#t" (or2 #t #t))
(test-assert "or2-#f-#f" (not (or2 #f #f)))

(test-assert "and2-#t-#f" (not (and2 #t #f)))
(test-assert "and2-#f-#t" (not (and2 #f #t)))
(test-assert "and2-#t-#t" (and2 #t #t))
(test-assert "and2-#f-#f" (not (and2 #f #f)))

(test-end "core")
