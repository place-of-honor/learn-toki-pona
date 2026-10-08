-- Actual modified Icky Lua. Embedded and parsed as these unchanged bytes.
-- Native hit testing, course parsing and all pixel work stay in C.
local reset_question_state ← ƒ(session)
  session.answered ← false
  session.choice ← -1
  session.mask ← 0
  session.left ← -1
  session.right ← -1
  session.miss ← false
end
local restart_session ← ƒ(session)
  session.question ← 0
  session.score ← 0
  session.screen ← 1
  reset_question_state(session)
end
local advance_question ← ƒ(session)
  if session.question + 1 ≥ session.length then
    session.screen ← 2
  else
    session.question ← session.question + 1
    reset_question_state(session)
  end
end
local choose_answer ← ƒ(session, chosen, correct)
  session.choice ← chosen
  session.answered ← true
  if chosen ≟ correct then session.score ← session.score + 1 end
end
local resolve_matching_selection ← ƒ(session)
  if session.left < 0 or session.right < 0 then return end
  local right_pair ← 3 - session.right
  if session.left ≟ right_pair then
    session.mask ← session.mask | (1 << session.left)
  else
    session.miss ← true
  end
  session.left ← -1
  session.right ← -1
  if session.mask ≟ 15 then
    session.answered ← true
    if not session.miss then session.score ← session.score + 1 end
  end
end
local transition ← ƒ(length, question, score, screen, answered, choice,
                      mask, left, right, miss, event, index, correct)
  local session ← {
    length ← length, question ← question, score ← score, screen ← screen,
    answered ← answered, choice ← choice, mask ← mask, left ← left,
    right ← right, miss ← miss
  }
  if event ≟ 0 then
    session.length ← index
    restart_session(session)
  elseif event ≟ 1 then
    restart_session(session)
  elseif event ≟ 2 then
    advance_question(session)
  elseif event ≟ 3 then
    choose_answer(session, index, correct)
  elseif event ≟ 4 then
    session.left ← index
    resolve_matching_selection(session)
  elseif event ≟ 5 then
    session.right ← index
    resolve_matching_selection(session)
  end
  return session.length, session.question, session.score, session.screen,
         session.answered, session.choice, session.mask, session.left,
         session.right, session.miss
end
return { version ← 1, transition ← transition }
