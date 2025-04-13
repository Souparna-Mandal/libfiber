// SPDX-FileCopyrightText: 2012-2023 Brian Watling <brian@oxbo.dev>
// SPDX-License-Identifier: MIT

#include "fiber_manager.h"
#include "test_helper.h"

void* run_function(void* param) {
  int* value = (int*)param;
  *value += 1;
  fiber_yield(0);
  *value += 1;
  return NULL;
}

int main() {
  _Atomic int y = 1;
  int z = atomic_exchange(&y, 2);
  test_assert(z == 1);
  test_assert(y == 2);

  fiber_manager_init(1);
  int volatile value = 0;
  fiber_t* fiber1 = fiber_create(20000, &run_function, (void*)&value);

  fiber_yield(0);
  test_assert(value == 1);
  fiber_join(fiber1, NULL);
  test_assert(value == 2);

  fiber_t* fiber2 = fiber_create(20000, &run_function, (void*)&value);

  fiber_yield(0);
  test_assert(value == 3);
  fiber_yield(0);
  test_assert(value == 4);

  // now the fiber has finished, but joining fiber2 should still be fine
  fiber_join(fiber2, NULL);

  // let fiber 2 do its maintenance (it needs to set state = DONE after we've
  // joined it)
  fiber_yield(0);

  fiber_manager_print_stats();
  fiber_shutdown();
  return 0;
}
