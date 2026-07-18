$$
\begin{aligned}
[\text{Prog}] &\to [\text{Stmt}]^*
\\[0.5em]

[\text{Stmnt}] &\to
\begin{cases}
\text{exit}([\text{Expr}]); & \\
\text{let}\ \text{ident} = [\text{Expr}]; & \\
\text{ident} = [\text{Expr}]; & \\
\text{if}([\text{Expr}])[\text{Scope}][\text{IfPred}] & \\
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

[\text{BinExpr}] &\to
\begin{cases}
[\text{Expr}] * [\text{Expr}] & \text{prec}=1 \\
[\text{Expr}] / [\text{Expr}] & \text{prec}=1 \\
[\text{Expr}] + [\text{Expr}] & \text{prec}=0 \\
[\text{Expr}] - [\text{Expr}] & \text{prec}=0
\end{cases}
\\[1em]

[\text{Term}] &\to
\begin{cases}
\text{int\_lit} & \\
\text{ident} & \\
([\text{Expr}]) &
\end{cases}

\end{aligned}
$$