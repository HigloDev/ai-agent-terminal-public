const SECRET_KEY_RE =
  /token|api[-_]?key|apikey|authorization|auth[-_]?header|password|passwd|secret|credential|bearer|private[-_]?key|access[-_]?key|refresh[-_]?token|session[-_]?token|cookie|discovery/i;

const REDACTED = '[REDACTED]';

/** Strip tokens/keys from free text before persisting or returning. */
export function redactText(text) {
  if (typeof text !== 'string' || !text) return text;
  let out = text;
  out = out.replace(/(Authorization:\s*)(Bearer|Basic)\s+\S+/gi, `$1$2 ${REDACTED}`);
  out = out.replace(/\bBearer\s+[A-Za-z0-9._~+/=-]{8,}/g, `Bearer ${REDACTED}`);
  out = out.replace(/\b(api[_-]?key|token|access[_-]?token|secret|password)\s*[=:]\s*["']?[^\s"']{6,}["']?/gi, `$1=${REDACTED}`);
  out = out.replace(/-----BEGIN [A-Z ]*PRIVATE KEY-----[\s\S]*?-----END [A-Z ]*PRIVATE KEY-----/g, REDACTED);
  return out;
}

export function redactObject(value, depth = 0) {
  if (depth > 8 || value == null) return value;
  if (Array.isArray(value)) return value.map((v) => redactObject(v, depth + 1));
  if (typeof value === 'object') {
    const out = {};
    for (const [k, v] of Object.entries(value)) {
      if (SECRET_KEY_RE.test(k)) out[k] = REDACTED;
      else out[k] = redactObject(v, depth + 1);
    }
    return out;
  }
  if (typeof value === 'string') return redactText(value);
  return value;
}

/** Reject path traversal for artifact/raw access. */
export function isSafeRelativePath(p) {
  if (typeof p !== 'string' || !p.trim()) return false;
  if (p.includes('\0')) return false;
  if (/^[a-zA-Z]:[\\/]/.test(p)) return false;
  if (p.startsWith('/') || p.startsWith('\\')) return false;
  if (p.split(/[\\/]/).some((seg) => seg === '..')) return false;
  return true;
}

export function assertSafeTaskBodyText(text) {
  const t = String(text ?? '');
  if (!t.trim()) {
    const err = new Error('任务内容不能为空');
    err.code = 'invalid_body';
    err.httpStatus = 400;
    throw err;
  }
  if (t.length > 8000) {
    const err = new Error('任务内容过长（上限 8000 字符）');
    err.code = 'invalid_body';
    err.httpStatus = 400;
    throw err;
  }
  return t;
}
