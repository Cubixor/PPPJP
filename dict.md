# PPPJP dictionary

```
<program> → <statement>* 
<statement> → { 
    kończwaść ( <expr> )
    zmienna <type> `ident` równa <expr> 
    `ident` równa <expr>
    <scope>
    jeśli ( <expr> ): <statement> 
        [przeciwnie jeśli ( <expr> ): <statement>] 
        [przeciwnie: <statement>]
    powtarzaj: <statement>/<loop_flow_stmt>
    powtarzaj jeśli ( <expr> ): <statement>/<loop_flow_stmt>
    
}
<loop_flow_stmt> → {
    przerwij
    kontynuuj
}
<scope> → {
    { <statement>* }
}
<type> → { całkowita }
<expr> → {
    <term>
    <arithmetic_expr>
}
<term> → {
    [integer_literal]
    `identifier`
}
<arithmetic_expr> → {
    (1) <expr> modulo <expr> 
    (1) <expr> razy <expr> 
    (1) <expr> podzielić <expr>
    (0) <expr> dodać <expr>
    (0) <expr> odjąć <expr>
}
```

## Code example //TODO

```
zmienna całkowita `a1` równa [dwa tysiące dwadzieścia cztery]

jeśliby `a1` było równe [pięć] to:
§1 wyświetlić ( "Tak było równo" )
przeciwnie:
§1 kończwaść [jeden]
```