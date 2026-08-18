

#ifdef MQSS_ENABLE_DEBUG
#define MQSS_DEBUG(X)                                                          \
  do {                                                                         \
    llvm::errs() << X;                                                         \
  } while (false)
#else
#define MQSS_DEBUG(X)                                                          \
  do {                                                                         \
  } while (false)
#endif
