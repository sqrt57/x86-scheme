; Core scheme functionality tests
;
; This file is part of x86-scheme project.
; Copyright (c) 2015-2017, Dmitry Grigoryev

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
(test-assert "symbol?-#t" (symbol? (quote df)))
(test-assert "symbol?-#f" (not (symbol? "dgf")))
(test-assert "procedure?-#t" (procedure? (lambda () (quote w))))
(test-assert "procedure?-#f" (not (procedure? (quote (lambda () (quote w))))))
(test-assert "procedure?-native" (procedure? car))
(test-assert "procedure?-continuation"
             (procedure? (call/cc (lambda (x) (x x)))))
(test-assert "number?-#t" (number? 25))
(test-assert "number?-#f" (not (number? (quote t))))
(test-assert "string?-#t" (string? "Hello world"))
(test-assert "string?-#f" (not (string? (quote t))))

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

(test-assert "string=?-#t" (string=? "abc" "abc"))
(test-assert "string=?-#f-1" (not (string=? "abc" "def")))
(test-assert "string=?-#f-2" (not (string=? "abc" "abcd")))

(test-eq "string->symbol" (quote abba) (string->symbol "abba"))
(test-eq "string->symbol" (quote cons) (string->symbol "cons"))
(test-assert "symbol->string" (string=? "abkm" (symbol->string (quote abkm))))

(test-assert "string-append-2" (string=? "Hello world"
                                         (string-append-2 "Hello " "world")))
(test-assert "string-append"
             (string=? "Hello, world"
                       (string-append "Hello" ", " "world")))
(test-assert "string-append-0" (string=? "" (string-append)))

(test-assert "memq-#t" (memq (quote a) (quote (c b a x))))
(test-assert "memq-#f" (not (memq (quote d) (quote (c b a x)))))

(test-assert "has-feature?-#t" (has-feature? (quote x86-scheme)))
(test-assert "has-feature?-#f" (not (has-feature? (quote test-feature))))
(add-feature! (quote test-feature))
(test-assert "add-feature!" (has-feature? (quote test-feature)))

(test-eq "assq-#t"
         (cadr (assq (quote h) (quote ((a b) (h i) (e f)))))
         (quote i))
(test-assert "assq-#f" (not (assq (quote h) (quote ((a b) (c d))))))

(test-assert "map" (= (cadr (map (lambda (x) (+ 10 x))
                            (quote (1 2))))
                   12))
(define l (quote ((a (b)) (c d))))
(define lc (alist-copy l))
(test-assert "alist-copy-new1" (not (eq? l lc)))
(test-assert "alist-copy-new2" (not (eq? (car l) (car lc))))
(test-eq "alist-same-key" (caar l) (caar lc))
(test-eq "alist-same-value" (cadar l) (cadar lc))

(define pair (quote (a . b)))
(set-car! pair (quote c))
(test-eq "set-car!" (car pair) (quote c))
(set-cdr! pair (quote d))
(test-eq "set-cdr!" (cdr pair) (quote d))

(define env (create-environment))
(test-eq "new-env-parent" (environment-parent env) #f)
(test-eq "new-env-bindings" (environment-bindings env) (quote ()))
(set-environment-parent! env (global-environment))
(test-eq "set-env-parent" (environment-parent env) (global-environment))
(define blist (quote ((a 1) (b 2))))
(set-environment-bindings! env blist)
(test-eq "set-env-bindings" (environment-bindings env) blist)

(test-eq "eval"
  (eval-current-environment (quote (car (quote (a . b))))) (quote a))

(test-eq "list" (cadr (list (quote a) (quote b) (quote c))) (quote b))
(test-eq "list-empty" (list) (quote ()))

(test-eq "lexical-scope"
  ((lambda (x) (((lambda (x) (lambda () x)) (quote lexical))))
    (quote dynamical))
  (quote lexical))

(define env #f)
((lambda (x y) (set! env (current-environment))) (quote one) (quote two))
(test-eq "current-environment" (cdar (environment-bindings env)) (quote one))

(define env (create-environment))
(set-environment-bindings!
  env
  (list (cons (quote plus) +)
        (cons (quote one) 1)
        (cons (quote two) 2)))
(test-assert "eval" (= (eval (quote (plus one two)) env) 3))

(test-assert "apply" (= (apply + (list 1 2 3)) 6))

(test-end "core")

(test-begin "write" 0)

(write (create-environment))
(write (lambda () 1))
(write (call/cc (lambda (x) (x x))))
(write +)
(write define)
(write if)

(test-end "write")
