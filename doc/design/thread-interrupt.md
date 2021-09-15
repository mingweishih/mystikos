# Thread Interrupt in Mystikos

On Linux, a process can interrupt a running thread by sending signals (via tkill/tgkill).
Mystikos currently simulates this operation as follows.
- A thread invokes tkill or tgkill syscall to send a signal to a target thread
- The Mystikos kernel invokes `myst_signal_deliver`, which appends the signal to
  the `myst_thread_t` structure associated with the thread.
- Upon the target thread is running and makes a syscall (after the invocation of `myst_signal_deliver`),
  the Mystikos kernel invokes `myst_signal_process` to handle the pending signal.

As described, the simulation of tkill/tgkill does not interrupt the thread but delays
the signal handling. As a result, Mystikos cannot support an application that requires
an "actual" interrupt.

# Design

This proposal aims to support thread interrupt in Mystikos and ensures the consistency
and compatibility with existing signal handling implementation.

To support thread interrupt, the Mystikos tkill/tgkill syscall implementation needs to
make an OCALL to the host, which sends the signal to the target thread via native tkill/tgkill.

Catching and handling the signal require cooperation between Mystikos and the Open Enclave (OE)
runtime. More specifically, the OE runtime needs to register the existing signal handler for
the corresponding signal. Also, the OE in-enclave exception handler needs to forward the
information of the signal to Mystikos. To not relying on the information sent from the host,
this document proposes the following design.

- Thread interruption
  - The OE host runtime registers the signal handler for a new signal, `SIGUSR1`.
  - The OE in-enclave exception handler forwards the following information to Mystkos
    - `oe_exception_record->code`: `OE_EXCEPTION_UNKNOWN`
    - Other fields: zero
    - Consideration: may affect the #PF simulation in SGX1 debug mode.
  - The Mystikos enclave adds logic to handle when the exception code equals to `OE_EXCEPTION_UNKNOWN`.

- Trusted signal information passing
  - The Mystikos kernel makes a tgkill OCALL after `myst_signal_delivery`, which
    - stores the `siginfo_t` inside the `myst_thread_t` structure of the target thread and
    - marks the target thread as interrupted
  - The signal handler invoked by the Mystikos enclave does the following
    - Check if the thread is interrupted.
      - If true, invoke `_handle_interrupt`, which invokes `handle_one_signal` using the stored
        siginfo.
      - Otherwise, invoke `_handle_one_signal`.
    - This part can potentially be consolidated with `myst_signal_process`.

 - Other considerations
   - Make the interrupt support as build-time (`MYST_THREAD_INTERRUPT`) or run-time (controlled via a function parameter) options.

# Implementation

  List of sample code snippets for Mystikos implementation:

  - Enclave
    ```c
    static uint64_t _forward_exception_as_signal_to_kernel(
        oe_exception_record_t* oe_exception_record)
    {
        ...
        if (oe_exception_code == OE_EXCEPTION_UNKNOWN)
        {
            (*_kargs.myst_handle_host_signal)(&siginfo, &_mcontext);
            _mcontext_to_oe_context(&_mcontext, oe_context);
            return OE_EXCEPTION_CONTINUE_EXECUTION;
        }
        ...
    ```

  - Syscall layer
    ```c
    long myst_syscall_tgkill(int tgid, int tid, int sig)
    {
        ...
        siginfo->si_code = SI_TKILL;
        siginfo->si_signo = sig
        siginfo->si_pid = tgid;
        myst_signal_delivery(target, sig, siginfo);
    #ifdef MYST_THREAD_INTERRUPT
        long params[6] = {(pid_t)tgid, (pid_t)target->target_tid, SIGUSR1};
        ret = (long)myst_tcall(SYS_tgkill, params);
    #endif
        ...
    }
    ```

  - Signal handling 
    ```c
    long myst_signal_delivery(
        myst_thread_t* thread,
        unsigned signum,
        siginfo_t* siginfo)
    {
        ...
        thread->signal.interrupt_siginfo = siginfo;
        thread->is_interrupted = true;
        ...
    }
    ```

    ```c
    static long _handle_interrupt(
        myst_thread_t* thread,
        mcontext_t* mcontext)
    {
        long ret = 0;
        siginfo_t* siginfo = thread->signal.interrupt_siginfo;

        ret = _handle_one_signal(siginfo->si_signo, siginfo, mcontext);

        thread->is_interrupted = false;
        thread->signal.interrupt_siginfo = NULL;

        return ret;
    }
    ```

    ```c
    long myst_handle_host_signal(siginfo_t* siginfo, mcontext_t* mcontext)
    {
        myst_thread_t* thread = myst_thread_self();
        if (thread->is_interrupted)
            return _handle_interrupt(thread, mcontext);
        return _handle_one_signal(siginfo->si_signo, siginfo, mcontext);
    }
    ```
