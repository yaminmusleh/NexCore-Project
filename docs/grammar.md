$$
\begin{aligned}
[\text{Prog}] &\to [\text{Stmt}]^*
\\[0.5em]

[\text{Stmnt}] &\to
\begin{cases}
\text{exit}([\text{Expr}]); & \\
\text{set}\ \text{ident} = [\text{Expr}]; & \\
\text{iff}([\text{Condition}])[\text{Scope}][\text{IfPred}] & \\
[\text{Scope}] &
\end{cases}
\\[1em]

[\text{Scope}] &\to
\{[\text{Stmt}]^*\}
\\[1em]

[\text{IfPred}] &\to
\begin{cases}
\text{elif}([\text{Expr}])[\text{Scope}][\text{IfPred}] & \\
\text{else}[\text{Scope}] & \\
\epsilon &
\end{cases}
\\[1em]

[\text{Expr}] &\to
\begin{cases}
[\text{Term}] & \\
[\text{BinExpr}] &
\end{cases}
\\[1em]

[\text{Condition}] &\to
\begin{cases}
[\text{Expr}] & \\
[\text{Expr}][\text{CompOp}][\text{Expr}] &
\end{cases}
\\[1em]

[\text{CompOp}] &\to
\begin{cases}
== & \\
!= & \\
< & \\
<= & \\
> & \\
>= &
\end{cases}
\\[1em]

[\text{BinExpr}] &\to
\begin{cases}
[\text{Expr}] * [\text{Expr}] & \text{prec}=1 \\
[\text{Expr}] / [\text{Expr}] & \text{prec}=1 \\
[\text{Expr}] + [\text{Expr}] & \text{prec}=0 \\
[\text{Expr}] - [\text{Expr}] & \text{prec}=0
\end{cases}
\\[1em]

[\text{Term}] &\to
[\text{Primary}]
\left\{
\begin{array}{l}
*[\text{Primary}] \\
/[\text{Primary}]
\end{array}
\right\}^*
\\[1em]

[\text{Primary}] &\to
\begin{cases}
\text{int\_lit} & \\
\text{ident} & \\
([\text{Expr}]) &
\end{cases}

\end{aligned}
$$