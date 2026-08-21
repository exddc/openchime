#ifndef OC_RUNTIME_SIGNAL_HANDLER_H
#define OC_RUNTIME_SIGNAL_HANDLER_H

#include <csignal>
#include <string>

namespace oc::runtime {

class SignalHandler {
  public:
    void Install();
    bool ShouldStop() const;
    int LastSignal() const;
    static std::string SignalName(int signal);

  private:
    static void Handle(int signal);
};

} // namespace oc::runtime

#endif
