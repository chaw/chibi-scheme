
#define SEXP_MAX_SIGNUM 32

static sexp sexp_signal_contexts[SEXP_MAX_SIGNUM];

static void sexp_call_sigaction (int signum, siginfo_t *info, void *uctx) {
  sexp ctx, sigctx, handler;
  sexp_gc_var1(args);
  ctx = sexp_signal_contexts[signum];
  if (ctx) {
    handler = sexp_vector_ref(sexp_global(ctx, SEXP_G_SIGNAL_HANDLERS),
                              sexp_make_fixnum(signum));
    if (sexp_truep(handler)) {
      sigctx = sexp_make_child_context(ctx, NULL);
      sexp_gc_preserve1(sigctx, args);
      args = sexp_cons(sigctx, SEXP_FALSE, SEXP_NULL);
      sexp_car(args)
        = sexp_make_cpointer(sigctx, sexp_siginfo_t_type_id, info, SEXP_FALSE, 0);
      args = sexp_cons(sigctx, SEXP_FALSE, args);
      sexp_car(args) = sexp_make_fixnum(signum);
      sexp_apply(sigctx, handler, args);
      sexp_gc_release1(sigctx);
    }
  }
}

static struct sigaction call_sigaction = {
  .sa_sigaction = sexp_call_sigaction,
  .sa_flags = SA_SIGINFO | SA_NODEFER
};

static struct sigaction call_sigdefault = {.sa_handler = SIG_DFL};
static struct sigaction call_sigignore = {.sa_handler = SIG_IGN};

static sexp sexp_set_signal_action (sexp ctx, sexp signum, sexp newaction) {
  int res;
  sexp oldaction;
  if (! (sexp_fixnump(signum) && sexp_unbox_fixnum(signum) > 0
         && sexp_unbox_fixnum(signum) < SEXP_MAX_SIGNUM))
    return sexp_type_exception(ctx, "not a valid signal number", signum);
  if (! (sexp_procedurep(newaction) || sexp_opcodep(newaction)
         || sexp_booleanp(newaction)))
    return sexp_type_exception(ctx, "not a procedure", newaction);
  if (! sexp_vectorp(sexp_global(ctx, SEXP_G_SIGNAL_HANDLERS)))
    sexp_global(ctx, SEXP_G_SIGNAL_HANDLERS)
      = sexp_make_vector(ctx, sexp_make_fixnum(SEXP_MAX_SIGNUM), SEXP_FALSE);
  oldaction = sexp_vector_ref(sexp_global(ctx, SEXP_G_SIGNAL_HANDLERS), signum);
  res = sigaction(sexp_unbox_fixnum(signum),
                  (sexp_booleanp(newaction) ?
                   (sexp_truep(newaction) ? &call_sigdefault : &call_sigignore)
                   : &call_sigaction),
                  NULL);
  if (res)
    return sexp_user_exception(ctx, SEXP_FALSE, "couldn't set signal", signum);
  sexp_vector_set(sexp_global(ctx, SEXP_G_SIGNAL_HANDLERS), signum, newaction);
  sexp_signal_contexts[sexp_unbox_fixnum(signum)] = ctx;
  return oldaction;
}

