# Source this in an interactive zsh session after setting MATRIXCLEAR_BIN.
# A function can expand the current prompt; an alias cannot pass it to the
# renderer, because zsh draws the next prompt only after the command returns.
unalias clear 2>/dev/null

function clear {
  local matrixclear_prompt
  matrixclear_prompt=$(print -Pn -- "${PROMPT-}")
  MATRIXCLEAR_FINAL_PROMPT="$matrixclear_prompt" command "${MATRIXCLEAR_BIN:-matrixclear}" "$@"
}

# For Ctrl-L while editing a command, preview the current input as well.
function matrixclear-screen {
  local matrixclear_prompt
  matrixclear_prompt=$(print -Pn -- "${PROMPT-}")
  zle -I
  MATRIXCLEAR_FINAL_PROMPT="${matrixclear_prompt}${BUFFER}" command "${MATRIXCLEAR_BIN:-matrixclear}"
  zle reset-prompt
}
if [[ -o interactive ]]; then
  zle -N matrixclear-screen
  bindkey '^L' matrixclear-screen
fi
