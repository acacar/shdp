#include <libgen.h>
#include <string.h>
#include "corpus.hpp"
#include "state.hpp"
#include "utils.hpp"

gsl_rng * RANDOM_NUMBER = NULL;
gsl_rng * RANDOM_NUMBER_ALT = NULL;

void print_usage_and_exit() {
  // print usage information

    printf("\nC++ implementation of Gibbs sampling for the Kernel Smoothed Hierarchical Dirichlet Process\n");
    printf("Author: Aybar C. Acar, acacar@metu.edu.tr, Grad. School of Informatics, Middle East Technical University.\n\n");
    printf("Forked from: https://github.com/Blei-Lab/hdp \n");
    printf("Original author: Chong Wang, chongw@cs.princeton.edu, Computer Science Department, Princeton University.\n\n");

    printf("usage:\n");
    printf("      shdp               [options]\n");
    printf("      --help:           print help information.\n");
    printf("      --verbose:        print running information.\n");
    printf("\n");

    printf("      control parameters:\n");
    printf("      --directory:       the saving directory, required.\n");
    printf("      --random_seed:     the random seed, default from the current time.\n");
    printf("      --max_iter:        the max number of iterations, default 100 (-1 means infinite).\n");
    printf("      --max_time:        the max time allowed (in seconds), default 1800 (-1 means infinite).\n");
    printf("      --burn_in:         number of burn-in iterations, default 50.\n\t\t\t\tSmart iteration and best likelihood will not work during the burn-in period.\n");
    printf("      --smart_iter:      Will terminate if best likelihood has not improved in smart_iter iterations after burn-in.\n \t\t\t\t(-1 means disabled, smart_iter will override max_iter).\n");
    printf("\n");

    printf("      data parameters:\n");
    printf("      --train_data:      the training data file/pattern, in lda-c format.\n");
    printf("\n");

    printf("      model parameters:\n");
    printf("      --eta:             the topic Dirichlet parameter, default 0.5.\n");
    printf("      --gamma:           the first-level concentration parameter in hdp, default 1.0.\n");
    printf("      --alpha:           the second-level concentration parameter in hdp, default 1.0.\n");
    printf("      %c[4m--rho_matrix:      the smoothing matrix, in space delimited format. %c[0m\n\t\t\t\tA matrix of ones will be used if not given\n",0x1B,0x1B );
    printf("      --gamma_a:         shape for 1st-level concentration parameter, default 1.0.\n");
    printf("      --gamma_b:         scale for 1st-level concentration parameter, default 1.0.\n");
    printf("      --alpha_a:         shape for 2nd-level concentration parameter, default 1.0.\n");
    printf("      --alpha_b:         scale for 2nd-level concentration parameter, default 1.0.\n");
    printf("      --sample_hyper:    sample 1st and 2nd-level concentration parameter, default false\n");
    printf("\n");

    printf("      test only parameters:\n");
    printf("      --test_data:       the test data file/pattern, in lda-c format.\n");
    printf("      --model_prefix:    the model_prefix.\n");

    printf("*******************************************************************************************************\n");
    exit(0);
}

int main(int argc, char* argv[]) {
  if (argc < 2) print_usage_and_exit();

  int verbose = 0;

  // Control parameters.
  char*  directory = NULL;
  time_t t; time(&t);
  long   random_seed = (long) t;
  int    max_iter = 100;
  int    max_time = 1800;
  int    save_lag = 5;
  int    burn_in = 50;
  int    smart_iter = -1;

  // Data parameters.
  char* train_data = NULL;
  char* rhomatrix_fn = NULL;

  // Model parameters.
  double eta = 0.5;
  double gamma = 1.0;
  double alpha = 1.0;
  double gamma_a = 1.0;
  double gamma_b = 1.0;
  double alpha_a = 1.0;
  double alpha_b = 1.0;
  int sample_hyper = 0;

  // test only parameters
  char* test_data = NULL;
  char* model_prefix = NULL;

  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--help")) print_usage_and_exit();
    else if (!strcmp(argv[i], "--verbose"))        verbose = 1;

    else if (!strcmp(argv[i], "--directory"))       directory = argv[++i];
    else if (!strcmp(argv[i], "--random_seed"))     random_seed = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--max_iter"))        max_iter = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--max_time"))        max_time = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--burn_in"))        burn_in = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--smart_iter"))      smart_iter = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--save_lag"))        save_lag = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--train_data"))      train_data = argv[++i];
    else if (!strcmp(argv[i], "--rho_matrix"))       rhomatrix_fn = argv[++i];
    else if (!strcmp(argv[i], "--eta"))             eta = atof(argv[++i]);
    else if (!strcmp(argv[i], "--gamma"))           gamma = atof(argv[++i]);
    else if (!strcmp(argv[i], "--alpha"))           alpha = atof(argv[++i]);
    else if (!strcmp(argv[i], "--gamma_a"))         gamma_a = atof(argv[++i]);
    else if (!strcmp(argv[i], "--gamma_b"))         gamma_b = atof(argv[++i]);
    else if (!strcmp(argv[i], "--gamma_a"))         gamma_a = atof(argv[++i]);
    else if (!strcmp(argv[i], "--gamma_b"))         gamma_b = atof(argv[++i]);
    else if (!strcmp(argv[i], "--sample_hyper"))    sample_hyper = 1;

    else if (!strcmp(argv[i], "--test_data"))       test_data = argv[++i];
    else if (!strcmp(argv[i], "--model_prefix"))    model_prefix = argv[++i];
    else {
      printf("%s, unknown parameters, exit\n", argv[i]);
      print_usage_and_exit();
    }
  }
  /// print information
  printf("************************************************************************************************\n");

  if (directory == NULL)  {
      printf("Following information is missing: --directory\n");
    printf("Run ./shdp for help.\n");
    exit(0);
  }

  if (!dir_exists(directory)) make_directory(directory);
  printf("Working directory: %s.\n", directory);

  char name[500];
  // Init random numbe generator.
  RANDOM_NUMBER = new_random_number_generator(random_seed);
  RANDOM_NUMBER_ALT = new_random_number_generator(random_seed+1);

  if (test_data == NULL || model_prefix == NULL) {

    if (train_data == NULL) {
      printf("Following information is missing: --train_data\n");
      printf("Run ./shdp for help.\n");
      exit(0);
    }
    
    if (smart_iter != -1)
      max_iter = -1;

    sprintf(name, "%s/settings.dat", directory);
    printf("Setting saved at %s.\n", name);
    FILE* setting_file = fopen(name, "w");

    fprintf(setting_file, "Control parameters:\n");
    fprintf(setting_file, "directory: %s\n", directory);
    fprintf(setting_file, "random_seed: %d\n", (int)random_seed);
    fprintf(setting_file, "save_lag: %d\n", save_lag);
    fprintf(setting_file, "max_iter: %d\n", max_iter);
    fprintf(setting_file, "max_time: %d\n", max_time);
    fprintf(setting_file, "burn_in: %d\n", burn_in);
    fprintf(setting_file, "smart_iter: %d\n", smart_iter);

    fprintf(setting_file, "\nData parameters:\n");
    fprintf(setting_file, "train_data: %s\n", train_data);
    fprintf(setting_file, "rho_matrix: %s\n", rhomatrix_fn);
    fprintf(setting_file, "\nModel parameters:\n");
    fprintf(setting_file, "eta: %.4lf\n", eta);
    fprintf(setting_file, "gamma: %.4lf\n", gamma);
    fprintf(setting_file, "alpha: %.4lf\n", alpha);
    fprintf(setting_file, "gamma_a: %.2lf\n", gamma_a);
    fprintf(setting_file, "gamma_b: %.4lf\n", gamma_b);
    fprintf(setting_file, "gamma_a: %.2lf\n", alpha_a);
    fprintf(setting_file, "gamma_b: %.4lf\n", alpha_b);
    fprintf(setting_file, "sample_hyper: %d\n", sample_hyper);

    fclose(setting_file);

    Corpus* c_train = NULL;

    printf("Reading training data from %s.\n", train_data);
    // Reading one of the train data.
    c_train = new Corpus();
    c_train->read_data(train_data);

    if (rhomatrix_fn == NULL)
        printf("Not rho matrix file given. Using unit rho matrix.\n");
    else
        printf("Reading rho matrix from %s.\n", rhomatrix_fn);


    // Start iterating.
    time_t start, current;
    int total_time = 0;
    int iter = 0;
    if (smart_iter != -1)
      {
        max_iter = burn_in + smart_iter;
      }
    HDP* hdp = new HDP();
    hdp->init_hdp(eta, gamma, alpha, c_train->size_vocab_, rhomatrix_fn);

    // Setting up the hdp state.
    hdp->setup_doc_states(c_train->docs_);
    // first iteration
    hdp->iterate_gibbs_state(false, false);

    // Open the log file for training data.
    sprintf(name, "%s/train.log", directory);
    FILE* train_log = fopen(name, "w");
    // Heldout columns record the documents that have not seen before.
    sprintf(name, "time\titer\tnum.topics\tgamma\talpha\t\tword.count\tlikelihood\tavg.likelihood\tbest.so.far");
    if(verbose) printf("%s\n", name);
    fprintf(train_log, "%s\n", name);

    double best_likelihood = -INFINITY;
    char best_so_far = ' ';
    while ((max_iter == -1 || iter < max_iter) && (max_time == -1 || total_time < max_time)) {
      ++iter;
      time (&start);

      // Iterations.
      hdp->iterate_gibbs_state(true, true);
      // Scoring the documents.
      double likelihood = hdp->log_likelihood(NULL);
      hdp->compact_hdp_state();

      if (sample_hyper) hdp->hyper_inference(gamma_a, gamma_b, alpha_a, alpha_b);

      // Record the time.
      time(&current);
      int elapse = (int) difftime(current, start);
      total_time += elapse;
      best_so_far = ' ';
      if (iter > burn_in)
        {
          if (best_likelihood < likelihood)
            {
              best_likelihood = likelihood; 
              sprintf(name, "%s/best", directory);
              hdp->save_state(name);
              hdp->save_doc_states(name);
              if (smart_iter != -1 )
                max_iter = iter + smart_iter;
              best_so_far = '*';
            }
        }

      sprintf(name, "%d\t%d\t%d\t\t%.5f\t%.5f\t\t%d\t\t%.3f\t%.5f\t%c",
              total_time, iter, hdp->hdp_state_->num_topics_, hdp->hdp_state_->gamma_,
              hdp->hdp_state_->alpha_, c_train->num_total_words_, likelihood, likelihood/c_train->num_total_words_, best_so_far);

      if (verbose) printf("%s\n", name);
      fprintf(train_log, "%s\n", name);
      fflush(train_log);


      if (save_lag > 0 && (iter % save_lag == 0)) {
        sprintf(name, "%s/iter@%05d", directory, iter);
        hdp->save_state(name);
      }
    }

    sprintf(name, "%s/final", directory);
    hdp->save_state(name);
    hdp->save_doc_states(name);

    // Free training data.
    if (c_train != NULL) {
      delete c_train;
    }
    fclose(train_log);

    delete hdp;
  }

  if (test_data != NULL && model_prefix != NULL) {
    Corpus* c_test = new Corpus();
    c_test->read_data(test_data);

    if (rhomatrix_fn == NULL)
      {
        printf("Not rho matrix file given. Using unit rho matrix.\n");
        //rho->create_unit_matrix(c_test->size_vocab_);
      }
    else
      {
        printf("Reading rho matrix from %s.\n", rhomatrix_fn);
        //rho->read_from_file(rhomatrix_fn);
      }
    //if (!rho->check(c_test->size_vocab_))
    //exit(0);

    HDP* hdp = new HDP();
    printf("Loading model from prefix %s...\n", model_prefix);
    hdp->load_state(model_prefix);
    
    // Remember the old state.
    HDPState* old_hdp_state = new HDPState();
    old_hdp_state->copy_hdp_state(*hdp->hdp_state_);

    hdp->setup_doc_states(c_test->docs_);

    if (verbose) printf("Initialization ...\n");
    hdp->iterate_gibbs_state(false, false);

    sprintf(name, "%s/%s-test.log", directory, basename(model_prefix));
    FILE* test_log = fopen(name, "w");
    sprintf(name, "time\titer\tnum.topics\tword.count\tlikelihood\tavg.likelihood");
    if(verbose) printf("%s\n", name);
    fprintf(test_log, "%s\n", name);

    time_t start, current;
    int total_time = 0;
    int iter = 0;

    // Iterations.
    while ((max_iter == -1 || iter < max_iter) && (max_time == -1 || total_time < max_time)) {
      ++iter;
      time (&start);
      hdp->iterate_gibbs_state(true, true);
      double likelihood = hdp->log_likelihood(old_hdp_state);
      hdp->compact_hdp_state();
      time(&current);
      int elapse = (int) difftime(current, start);
      total_time += elapse;

      sprintf(name, "%d\t%d\t%d\t\t%d\t\t%.3f\t%.5f",
              total_time, iter, hdp->hdp_state_->num_topics_,
              c_test->num_total_words_, likelihood,
              likelihood/c_test->num_total_words_);

      if (verbose) printf("%s\n", name);
      fprintf(test_log, "%s\n", name);
      fflush(test_log);
    }

    if (verbose) printf("Done and saving ...\n");
    sprintf(name, "%s/%s-test", directory, basename(model_prefix));
    hdp->save_state(name);
    hdp->save_doc_states(name);
    fclose(test_log);

    delete hdp;
    delete old_hdp_state;
    delete c_test;
  }


  // Free random number generator.
  free_random_number_generator(RANDOM_NUMBER);
  free_random_number_generator(RANDOM_NUMBER_ALT);
  return 0;
}
