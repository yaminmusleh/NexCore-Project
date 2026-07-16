$$
\begin{aligned}
\langle \text{Program} \rangle
    &\to \langle \text{Statement} \rangle^*
\\
\langle \text{Statement} \rangle
    &\to \langle \text{ExitStatement} \rangle
    \mid \langle \text{VariableDeclaration} \rangle
\\
\langle \text{ExitStatement} \rangle
    &\to \texttt{exit} \; ( \langle \text{Expression} \rangle ) \; \texttt{;}
\\
\langle \text{VariableDeclaration} \rangle
    &\to \texttt{let} \; \texttt{identifier} \; = \; \langle \text{Expression} \rangle \; \texttt{;}
\\
\langle \text{Expression} \rangle
    &\to \langle \text{IntegerLiteral} \rangle
    \mid \texttt{identifier}
\\
\langle \text{IntegerLiteral} \rangle
    &\to \texttt{int\_lit}
\end{aligned}
$$