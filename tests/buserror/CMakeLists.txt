
set(testrunner ${CMAKE_CURRENT_SOURCE_DIR}/run_test.sh)

foreach(machine st megast ste megaste tt falcon)
 add_test(NAME buserror-${machine}-b COMMAND ${testrunner} $<TARGET_FILE:hatari> b ${machine})
 add_test(NAME buserror-${machine}-w COMMAND ${testrunner} $<TARGET_FILE:hatari> w ${machine})
endforeach(machine)

if(ENABLE_WINUAE_CPU)
 foreach (lvl RANGE 0 4)
  add_test(NAME buserror-680${lvl}0-fast
           COMMAND ${testrunner} $<TARGET_FILE:hatari> w st --cpulevel ${lvl} --compatible false --cpu-exact false)
  add_test(NAME buserror-680${lvl}0-compatible
           COMMAND ${testrunner} $<TARGET_FILE:hatari> w st --cpulevel ${lvl} --compatible true --cpu-exact false)
  add_test(NAME buserror-680${lvl}0-cpuexact
           COMMAND ${testrunner} $<TARGET_FILE:hatari> w st --cpulevel ${lvl} --compatible false --cpu-exact true)
  if (${lvl} GREATER 2)
  add_test(NAME buserror-680${lvl}0-mmu
           COMMAND ${testrunner} $<TARGET_FILE:hatari> w tt --cpulevel ${lvl} --mmu true)
  endif()
 endforeach(lvl)
 add_test(NAME buserror-68060
          COMMAND ${testrunner} $<TARGET_FILE:hatari> w tt --cpulevel 6 --mmu false)
 add_test(NAME buserror-68060-mmu
          COMMAND ${testrunner} $<TARGET_FILE:hatari> w tt --cpulevel 6 --mmu true)
endif(ENABLE_WINUAE_CPU)
