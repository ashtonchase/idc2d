#ifndef _OMP_H_DEF
#define _OMP_H_DEF
/*
* define the lock data types
*/
#ifndef _OMP_LOCK_T_DEF
# define _OMP_LOCK_T_DEF
typedef struct __omp_lock *omp_lock_t;
#endif
#ifndef _OMP_NEST_LOCK_T_DEF
# define _OMP_NEST_LOCK_T_DEF
typedef struct __omp_nest_lock *omp_nest_lock_t;
#endif
/*
* exported OpenMP functions
*/
#ifdef __cplusplus
extern "C" {
#endif
#if defined(__stdc__) || defined(__STDC__) || defined(__cplusplus)
extern void omp_set_num_threads(int num_threads);
extern int omp_get_num_threads(void);
extern int omp_get_max_threads(void);
extern int omp_get_thread_num(void);
extern int omp_get_num_procs(void);
extern int omp_in_parallel(void);
extern void omp_set_dynamic(int dynamic_threads);
extern int omp_get_dynamic(void);
extern void omp_set_nested(int nested);
extern int omp_get_nested(void);
extern void omp_init_lock(omp_lock_t *lock);
extern void omp_destroy_lock(omp_lock_t *lock);
extern void omp_set_lock(omp_lock_t *lock);
extern void omp_unset_lock(omp_lock_t *lock);
extern int omp_test_lock(omp_lock_t *lock);
extern void omp_init_nest_lock(omp_nest_lock_t *lock);
extern void omp_destroy_nest_lock(omp_nest_lock_t *lock);
extern void omp_set_nest_lock(omp_nest_lock_t *lock);
extern void omp_unset_nest_lock(omp_nest_lock_t *lock);
extern int omp_test_nest_lock(omp_nest_lock_t *lock);
extern double omp_get_wtime(void);
extern double omp_get_wtick(void);
#else
extern void omp_set_num_threads();
extern int omp_get_num_threads();
extern int omp_get_max_threads();
extern int omp_get_thread_num();
extern int omp_get_num_procs();
extern int omp_in_parallel();
extern void omp_set_dynamic();
extern int omp_get_dynamic();
extern void omp_set_nested();
extern int omp_get_nested();
extern void omp_init_lock();
extern void omp_destroy_lock();
extern void omp_set_lock();
extern void omp_unset_lock();
extern int omp_test_lock();
extern void omp_init_nest_lock();
extern void omp_destroy_nest_lock();
extern void omp_set_nest_lock();
extern void omp_unset_nest_lock();
extern int omp_test_nest_lock();
extern double omp_get_wtime();
extern double omp_get_wtick();
#endif
#ifdef __cplusplus
}
#endif
#endif