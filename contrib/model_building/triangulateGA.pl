#!/usr/bin/perl -w

## @file
##############################################################################
# triangulateGA
#
# Script for generating and timing GMTK triangulations
##############################################################################

use strict;
use File::Basename;
use File::Copy;
use File::Temp qw/ tempfile mktemp /;
use Getopt::Long;
use IO::Handle; 
use IPC::Open2; 
use Pod::Usage; 
use Symbol; 
use threads; 
use threads::shared;
use Thread::Semaphore;
use Thread::Queue;
use FindBin;

##############################################################################
# Executable names 
##############################################################################

my $global_triangulation_mode : shared = 'C';

my $architecture = `uname`;
chomp $architecture;

#my $ROOTDIR = $ENV{'ROOTDIR'};
#my $PROJECTDIR = $ENV{'PROJECTDIR'};
my $PROJECTDIR = '/cworkspace/ifp-32-1/hasegawa/programs';
my $SCRIPTSDIR = "$PROJECTDIR/scripts/gmtkScripts/scripts";
my $BINDIR = "$PROJECTDIR/gmtk/bin.$architecture/";
my $gmtk_triangulate ="$BINDIR/gmtkTriangulate";
my $gmtk_tfmerge = "$BINDIR/gmtkTFmerge";
my $gmtk_time = "$BINDIR/gmtkTime";
my $gmtk_args_h = "/cworkspace/ifp-32-1/hasegawa/akantor/gmtk_build/gmtk_dev/tksrc/GMTK_Arguments.h";

my $minimum_popsize = 32;
my $shrinkrate = 1.5;

my $crossover_prob = 0.2;
my $mutate_prob = 0.7; 

my $inference_opt_crossover_prob : shared = 0.5;
my $inference_opt_mutate_prob : shared = 0.20; 

##############################################################################
#  
##############################################################################
my @trifile_pool : shared;

##############################################################################
# State variables for basic methods 
##############################################################################
my $basic_once_index    : shared = 0;
my $advanced_once_index : shared = 0;

my @run_once = ( 'completed', 'copy' );
my @basic_iterations : shared = ( '100-', '1-' ); 
my @basic_prefix     : shared = ( '', 'pre-edge-all-', 'pre-edge-random-');
my @basic_suffix     : shared = ( '-1', '-3' );
my @basic_method : shared = ( 'MCS', 'R', 'W' );
my $basic_suffix_threshold : shared = 2;

my $basic_iteration_index : shared = 0;
my $basic_prefix_index    : shared = 0;
my $basic_method_index    : shared = 0;
my $basic_suffix_index    : shared = 0;
my $basic_iterations_done : shared = 0;

my @timing_option_list : shared = ( 'COT', 'COM', 'COI' ); 

my $boundary_searches_done : shared = 0;

my $nmbr_timing_processes : shared = 0;
my $nmbr_extra_triangulation_processes : shared = 0;

my $start_time; 
my $cumulative_time : shared = 0; 

my $numBoundaries : shared;

my $INFINITY = 1e20;
my $gmtkTime_timeout = 600;#this value could vary depending on short/medium/long

##############################################################################
# State variables for advanced methods 
##############################################################################
my @advanced_UB_types   : shared = ( 'F', 'T' );
my @advanced_iterations : shared = ( '100-', '10-', '1-' ); 
my @advanced_prefix     : shared = ( '', 'pre-edge-lo-', 'pre-edge-all-', 
  'pre-edge-random-');
my @advanced_method     : shared = ( 'frontier', 'MCS', 'R', 'T', 'X', 'S', 
  'F', 'W', 'P', 'N', 'ST', 'SF', 'SW', 'SFW', 'TS', 'TF', 'TW', 'TSW', 'FS', 
  'FT', 'FW', 'FTSW' );
my @advanced_suffix     : shared = ( '-1', '-2', '-3' );
my $advanced_suffix_threshold : shared = 2;

my $advanced_UB_type_index   : shared = 0;
my $advanced_iteration_index : shared = 0;
my $advanced_prefix_index    : shared = 0;
my $advanced_method_index    : shared = 0;
my $advanced_suffix_index    : shared = 0;
my $advanced_iterations_done : shared = 0;

my $method_count : shared = 1;

my $warning_message = "***WARNING: ";
my $error_message   = "***ERROR: ";

##############################################################################
# Initial gmtkTime options 
##############################################################################
my $initial_vcap = 'COT'; 
my $initial_component_cache = 'T'; 

##############################################################################
# Semaphores and thread safe queues  
##############################################################################
my $trifile_queue  = Thread::Queue->new; 
my $boundary_queue = Thread::Queue->new; 

my $best_sem = new Thread::Semaphore;
my $needed_trifiles    = new Thread::Semaphore(0);

my $timing_thread_idle_sem = new Thread::Semaphore(0);
my $timing_thread_idle : shared; 
my $basic_method_queue_in_use : shared; 
my $advanced_method_queue_in_use : shared; 
my $trifile_pool_in_use : shared; 

##############################################################################
# State variables for comparing triangulation methods 
##############################################################################
my $best_trifile    : shared = 0; 
my $best_partitions : shared = 0; 
my $best_boundary_index : shared = 'B0'; 
my $best_vcap : shared = $initial_vcap; 
my @best_time_per_boundary : shared;
my $trifile_count : shared = 0; 
my %job_status : shared = ();
my $EXECUTING = 1;
my $TERMINATED = 2;

##############################################################################
# Command line parameters 
##############################################################################
my @input_triangulations; 
my $str_file; 
my $timing_script;
my $short_mode;
my $medium_mode;
my $long_mode;
my $parallelism;
my $timing_export_line;
my $timing_length = 10; 
my $boundary_export_line;
my $triangulation_export_line;
my $output_directory;
my $str_basename;
my $working_dir;
my $use_existing_boundaries;
my $maximum_boundary_searches;
my $verbosity;
my $boundary_time;
my $triangulateP;
my $triangulateC;
my $triangulateE;
my $throttling = 1;

##############################################################################
# Number of iterations 
##############################################################################

my $C_iterations;
my $E_iterations;
my $P_iterations;

##############################################################################
# Description of the gmtkTime process when running in single processor mode 
##############################################################################
my $single_processor_timing_script_input;  
my $single_processor_timing_script_output;
my $single_processor_current_pid;
my $single_processor_vcap;
my $single_processor_caching;

##############################################################################
# Run the program 
##############################################################################

main();
print "Done Triangulating\n";


##############################################################################
# main 
#
# Evaluate command line parameters and start threads 
##############################################################################
sub main
{
  my $i; 
  my $valid_options;
  my @children;

  ##############################################################################
  # Get command line parameters 
  ##############################################################################
  my %gmtkTime_options;
  my %get_options_hash = ( 
    "strFile:s"                   => \$str_file,
    "short!"                      => \$short_mode,
    "medium!"                     => \$medium_mode,
    "long!"                       => \$long_mode,
    "timingExportLine:s"          => \$timing_export_line,
    "boundaryExportLine:s"        => \$boundary_export_line,
    "triangulationExportLine:s"   => \$triangulation_export_line,
    "timingScript:s"              => \$timing_script,
    "parallelism:i"               => \$parallelism,
    "outputDirectory:s"           => \$output_directory,
    "seconds:i"                   => \$timing_length,
    "useExistingBoundaries!"      => \$use_existing_boundaries,
    "maximumBoundarySearches:i"   => \$maximum_boundary_searches,
    "boundaryTime:s"              => \$boundary_time,
    "throttling:i"                => \$throttling,
    "verbosity:i"                 => \$verbosity,
    "triangulateP!"               => \$triangulateP,
    "triangulateC!"               => \$triangulateC,
    "triangulateE!"               => \$triangulateE
  );

  get_gmtkTime_options( \%get_options_hash, \%gmtkTime_options ); 

  $valid_options = &GetOptions( %get_options_hash );

  if (!$valid_options) {
    pod2usage( -verbose=>1 );
  }

  @input_triangulations = @ARGV;

  my $input_triangulation;
  foreach $input_triangulation (@input_triangulations)
  {
    if (! -e $input_triangulation)
    {
      pod2usage(-message=>"${error_message}Input triangulation '$input_triangulation' does not exist\n", -verbose=>0 );
    }
  }

  ##########################################################################
  # Process command line parameters
  ##########################################################################

  ########################################################################
  # Settings that don't deal with triangulation options
  ########################################################################
  if (!defined $str_file)
  {
    pod2usage(-message=>"${error_message}Must supply a structure file name\n",
      -verbose=>0 );
  }
  else 
  {
    (-e $str_file) or pod2usage(-message=>
      "${error_message}Stucture file '$str_file' does not exist\n", -verbose=>0
      );
  }

  if (!defined $timing_export_line) 
  {
    $timing_export_line = ' ';
  }

  if (!defined $boundary_export_line) 
  {
    $boundary_export_line = '/bin/bash'; 
  }

  if (!defined $triangulation_export_line) 
  {
    $triangulation_export_line = ' '; 
  }

  if (!defined $timing_script) 
  { 
    $timing_script = make_gmtkTime_script( \%gmtkTime_options );
  }

  if (!defined $output_directory)
  {
    print "${warning_message}No outputDirectory was supplied, using current directory\n";
    $output_directory = './';
  }

  if (! -d $output_directory)  
  {
    pod2usage(-message=>
      "${error_message}'$output_directory' is not a valid directory\n", 
      -verbose=>0
    );
  }

  if (!defined $throttling || $throttling < 0) 
  {
     $throttling = 1;
  }

  if (!defined $parallelism) 
  {
     $parallelism = 1;
  }

  if ($parallelism <= 0) 
  {
    pod2usage(-message=>
      "${error_message}Parallelism must be > 0\n", -verbose=>0 );
  }

  $nmbr_timing_processes = int($parallelism/2); 
  $nmbr_extra_triangulation_processes = $parallelism - 2*int($parallelism/2); 

  if (!defined $use_existing_boundaries) 
  {
    $use_existing_boundaries = 0;  
  }

  if (!defined $verbosity) 
  {
    $verbosity = 0;  
  }

  ########################################################################
  # Use the mode to set up default triangulation settings  
  ########################################################################
  if (!defined $short_mode) 
  {
    $short_mode = 0;
  }

  if (!defined $medium_mode) 
  {
    $medium_mode = 0;
  }

  if (!defined $long_mode) 
  {
    $long_mode = 0;
  }

  my $nmbr_modes;
  $nmbr_modes = $short_mode + $medium_mode + $long_mode;

  if ($nmbr_modes == 0)
  {
    $short_mode = 1;
  }

  if ($nmbr_modes > 1) 
  {
    pod2usage(-message=>
      "${error_message}Only use one of -short, -medium, and -long",
      -verbose=>0
    );
  }

  ########################################################################
  # Short mode 
  ########################################################################
  if ($short_mode) 
  {
    if (!defined $boundary_time)
    {
      $boundary_time = '36seconds';
    }

    if (!defined $triangulateP)
    {
      $triangulateP = 0;
    }

    if (!defined $triangulateC)
    {
      $triangulateC = 1;
    }

    if (!defined $triangulateE)
    {
      $triangulateE = 0;
    }

    $C_iterations = 0;
    $E_iterations = 0;
    $P_iterations = 0;
  }

  ########################################################################
  # Medium mode 
  ########################################################################
  elsif ($medium_mode)
  {
    if (!defined $boundary_time)
    {
      $boundary_time = '24minutes';
    }
 
    if (!defined $triangulateP)
    {
      $triangulateP = 1;
    }

    if (!defined $triangulateC)
    {
      $triangulateC = 1;
    }

    if (!defined $triangulateE)
    {
      $triangulateE = 1;
    }

    $C_iterations = 30;
    $E_iterations = 0;
    $P_iterations = 0;
  }

  ########################################################################
  # Long mode 
  ########################################################################
  elsif ($long_mode)
  {
    if (!defined $boundary_time)
    {
      $boundary_time = '4hours';
    }

    if (!defined $triangulateP)
    {
      $triangulateP = 1;
    }

    if (!defined $triangulateC)
    {
      $triangulateC = 1;
    }

    if (!defined $triangulateE)
    {
      $triangulateE = 1;
    }

    $C_iterations = 50;
    $E_iterations = 30;
    $P_iterations = 10;
  }

  #############################################################################
  # Initialize other global variables 
  #############################################################################
  $str_basename = basename($str_file); 
  $working_dir = `pwd`;
  chomp($working_dir);

  for ($i=0; $i<60; $i++)
  {
    $best_time_per_boundary[$i] = 0;
  }

  STDOUT->autoflush(1);
  $SIG{PIPE} = 'IGNORE';

  #############################################################################
  # Test parameters
  #############################################################################
  print "------- Checking Parameters -------\n";
  check_timing_and_export_parameters();

  #############################################################################
  # Start the threads 
  #############################################################################
  print "------- Triangulating $str_file -------\n"; 

  #########################################################################
  # Start gmtkTime locally if only using one processor 
  #########################################################################
  if ($parallelism)
  {
    $single_processor_timing_script_input  = gensym;
    $single_processor_timing_script_output = gensym;
    $single_processor_vcap = $initial_vcap;
    $single_processor_caching = $initial_component_cache;

    ($single_processor_current_pid, $single_processor_timing_script_input, 
      $single_processor_timing_script_output) = open_time(); 
  }

  #########################################################################
  # Start boundary searches 
  #########################################################################
  if (!$use_existing_boundaries) 
  {
    start_boundary_searches();
  }

  $start_time = time();

  while ($boundary_queue->pending > 0) {
    $boundary_queue->dequeue();
  }
  get_existing_boundaries();

  #########################################################################
  # Start thread to generate triangulations 
  #########################################################################
  printf "%-8s%-3s%-39s%-5s%-2s %4s\n", 'Time', 'B', 'Method', 'vcap', 'C', 
    'Partitions';

  #########################################################################
  # Start threads to time triangulations
  #########################################################################
  if ($parallelism > 1)
  {
    for ($i=0; $i<$nmbr_timing_processes; $i++) {
      push @children, threads->new(\&time_triangulation_thread );
      sleep $throttling;
    }
  }

  #########################################################################
  # Triangulate C
  #########################################################################
  $global_triangulation_mode = 'C';

  $needed_trifiles->up( $nmbr_extra_triangulation_processes );
  generate_initial_pool();

  genetic_triangulations( $C_iterations );

  #########################################################################
  # Triangulate E 
  #########################################################################
  if ($triangulateE)
  {
    print "--------------- Triangulating E ---------------\n";
    $global_triangulation_mode = 'E';
    $inference_opt_crossover_prob = 0.0;
    $inference_opt_mutate_prob    = 0.0; 
    @timing_option_list = ( $best_vcap ); 

    send_timing_thread_message('REBOOT'); 
      
    $best_partitions = 0; 
    generate_pool_from_files( ("${best_trifile}____${best_vcap}") );
    $boundary_queue->enqueue( "${best_trifile}" );
    $boundary_queue->enqueue( 'BOUNDARY_SEARCHES_DONE' ); 
    generate_initial_pool();
  
    genetic_triangulations( $E_iterations );
  }

  #########################################################################
  # Triangulate P 
  #########################################################################
  if ($triangulateP)
  {
    print "--------------- Triangulating P ---------------\n";
    $global_triangulation_mode = 'P';
  
    send_timing_thread_message('REBOOT'); 
  
    $best_partitions = 0; 
    generate_pool_from_files( ("${best_trifile}____${best_vcap}") );
    $boundary_queue->enqueue("${best_trifile}" ); 
    $boundary_queue->enqueue( 'BOUNDARY_SEARCHES_DONE' ); 
    generate_initial_pool();
  
    genetic_triangulations( $P_iterations );
  }

  #########################################################################
  # Tell the timing threads to exit 
  #########################################################################
  send_timing_thread_message('EXIT'); 

  #########################################################################
  # Wait for all threads to return 
  #########################################################################
  print STDERR "Waiting for all threads to return...";
  my $child;
  foreach $child (@children) {
    $child->join;
  }
  print "they're done\n";

}

#############################################################################
# genetic_triangulations
#
# 
#############################################################################
sub genetic_triangulations 
{
  my @method_list;
  my $iteration;
  my $last_iteration;
  my $prevbest_trifile = "none";
  my $prevbest_score = 0;

  $last_iteration = pop;

  ## Calculate best from initial pool for use in extra eliteism selection 
  for (my $i=1;$i<scalar(@trifile_pool);$i+=2){
    my $score = $trifile_pool[$i];
    if ($score > $prevbest_score){
      $prevbest_trifile = $trifile_pool[$i-1];
      $prevbest_score = $trifile_pool[$i];
    }
  } 

  infoMsg(20, "Crossover: $crossover_prob / Mutate: $mutate_prob\n");

  for ($iteration=1; $iteration<=$last_iteration; $iteration++)
  {
    infoMsg(10, "\n======================================================================\n");
    infoMsg(10, "Iteration $iteration\n");
    infoMsg(10, "======================================================================\n");

    my $desired_popsize;
    my $separateGA = 1;
    my $popsize;

    if ($iteration > 4){
      $separateGA = 0;
    }

    if ($separateGA){
      $desired_popsize = $numBoundaries * 20; # TODO: currently
                                              # hardcode this
    }
    else {
      $popsize = scalar(@trifile_pool) / 2; # because 2 entries rep. 1 gene
      $desired_popsize = int($popsize/$shrinkrate); # shrink to the minimum at shrinkrate 
      if ($desired_popsize < $minimum_popsize)
      {
        $desired_popsize = $minimum_popsize; 
      }
    }

    trifile_pool_selection('deathmatch','elitist',$desired_popsize,$separateGA);
    crossover($crossover_prob);

    ## EXTRA ELITIST ##
    ## Bring back the best previous gene if nothing in
    ## the current pool is better (this is separate from the elitism
    ## in the selection operator)
    my $currbest_score = 0;
    my $currbest_trifile = "none";
    for (my $i=1;$i<scalar(@trifile_pool);$i+=2){
	my $score = $trifile_pool[$i];
	if ($score > $currbest_score){
	    $currbest_trifile = $trifile_pool[$i-1];
	    $currbest_score = $trifile_pool[$i];
	}
    }

    if ($currbest_score < $prevbest_score){
	my $randindex = 2 * int(rand(($#trifile_pool/2)));
	$trifile_pool[$randindex] = $prevbest_trifile;
	$trifile_pool[$randindex+1] = $prevbest_score;
	print "Performed Extra Elitism\n";
    }
    else {
	$prevbest_score = $currbest_score;
	$prevbest_trifile = $currbest_trifile;
    }
  }

}



#############################################################################
# crossover
#
# Main GA crossover operator. First operator is crossover probability
#############################################################################
sub crossover
{
  my $crossover_prob = shift;
  my %boundary2trifile_list = ();
  my $boundary_name;
  my $boundary;
  my $boundary_M_type;
  my $boundary_D_type;
  my $tri_command_line;
  my @method_list;
  
  my $parent1_name; 
  my $parent1_vcap; 
  my $parent2_name; 
  my $parent2_vcap; 
  my $child1;
  my $child2;

  infoMsg(25,"crossover: called with $crossover_prob\n");

  ############################################################
  # Grep for boundary name and put all trifiles of the same
  # boundary in the same hash (which points to a list)
  ############################################################
  for(my $i=0; $i<(scalar(@trifile_pool)); $i+=2)
  {
    my $trifile_name = $trifile_pool[$i];
    my $trifile_partitions = $trifile_pool[$i+1];

    $trifile_name =~ /(B\d+)\./;
    $boundary_name = $1;
    push @{$boundary2trifile_list{$boundary_name}}, $i;
  }

  ############################################################
  # Crossover files from each boundary seperately 
  ############################################################
  foreach my $boundary (keys %boundary2trifile_list)
  {
    my @trifile_list = @{$boundary2trifile_list{$boundary}};

    # randomly shuffle the array, then pick successive elements in
    # the list as pairs for a crossover operation. All genes that
    # are not picked for crossover are simply propagated to the
    # next generation without modification.
    shuffle_array(\@trifile_list);
  
    for (my $i=0;$i<$#trifile_list;$i+=2)
    {
      my $parent1 = $trifile_pool[$trifile_list[$i]];
      my $parent2 = $trifile_pool[$trifile_list[$i+1]];
  
      ($parent1_name, $parent1_vcap) = $parent1 =~ /^(.+)____(.+)$/;
      ($parent2_name, $parent2_vcap) = $parent2 =~ /^(.+)____(.+)$/;

      #####################################################################
      # Get boundary type from the parent file name (both parents should match) 
      #####################################################################
      ($boundary_M_type) = $parent1_name =~ /([^\/]+)$/;
      ($boundary_M_type, $boundary_D_type) = $boundary_M_type =~ 
        /[^\.]+\.[^\.]+\.(M\d+)\.[^\.]+\.(D_[^\.]+)\./;
      if ((!defined $boundary_M_type) || (!defined $boundary_D_type)) {
        ($boundary_M_type) = $parent1_name =~ /([^\/]+)$/;
        ($boundary_M_type, $boundary_D_type) = $boundary_M_type =~ 
          /B\d+\.(M\d+)\.(D_[^\.]+)\./;
      }

      ############################################################
      # Crossover vcap option
      ############################################################
      if (rand(1) < $inference_opt_crossover_prob) {
        my $tmp;
        $tmp = $parent1_vcap; 
        $parent1_vcap = $parent2_vcap;
        $parent2_vcap = $tmp; 
      }
  
      ############################################################
      # mutate vcap option
      ############################################################
      if (rand(1) < $inference_opt_mutate_prob) {
        my $new_vcap;
        do { 
          $new_vcap = $timing_option_list[int(rand(scalar @timing_option_list))];
        } while ($new_vcap eq $parent1_vcap);
             
        $parent1_vcap = $new_vcap;
      } 
  
      if (rand(1) < $inference_opt_mutate_prob) {
        my $new_vcap;
        do { 
          $new_vcap = $timing_option_list[int(rand(scalar @timing_option_list))];
        } while ($new_vcap eq $parent2_vcap);
             
        $parent2_vcap = $new_vcap;
      } 

      $child1 = "$output_directory/$boundary.$boundary_M_type.$boundary_D_type.crossover.$i.A.$str_basename.trifile";
      $child2 = "$output_directory/$boundary.$boundary_M_type.$boundary_D_type.crossover.$i.B.$str_basename.trifile";

      my @trifile_names = ( $child1, $child2 );
      my @timing_option_strings = ( $parent1_vcap, $parent2_vcap );
  
      $tri_command_line = "-triangulationHeuristic crossover -outputTriangulatedFile $child1 -inputCrossoverTriangulatedF $parent2_name -outputCrossover $child2 -crossoverProb $crossover_prob -mutateProb $mutate_prob";
  
      push @method_list, {
        'METHOD_NAME'=>'crossover', 
        'TRI_COMMAND_LINE'=>$tri_command_line, 
        'BASE_TRIANGULATION'=>$parent1_name, 
        'TRIFILE_NAMES'=>\@trifile_names, 
        'TIMING_OPTION_STRINGS'=>\@timing_option_strings,
        'TRIANGULATION_MODE'=>$global_triangulation_mode };
    } # for $i in trifile_list  

    #####################################################################
    # If list has odd number of elements, mutate the last gene using
    # one-edge
    #####################################################################
    if (((scalar(@trifile_list)) % 2) == 1) {
print "================> using one-edge ===================\n";
      ($parent1_name, $parent1_vcap) = 
        $trifile_pool[$trifile_list[$#trifile_list]] =~ /^(.+)____(.+)$/;
      #####################################################################
      # Get boundary type from the parent file name (both parents should match) 
      #####################################################################
      ($boundary_M_type) = $parent1_name =~ /([^\/]+)$/;
      ($boundary_M_type, $boundary_D_type) = $boundary_M_type =~ 
        /[^\.]+\.[^\.]+\.(M\d+)\.[^\.]+\.(D_[^\.]+)\./;
      if ((!defined $boundary_M_type) || (!defined $boundary_D_type)) {
        ($boundary_M_type) = $parent1_name =~ /([^\/]+)$/;
        ($boundary_M_type, $boundary_D_type) = $boundary_M_type =~ 
          /B\d+\.(M\d+)\.(D_[^\.]+)\./;
      }
      $child1 = "$output_directory/$boundary.$boundary_M_type.$boundary_D_type.one-edge.$str_basename.trifile";
      $tri_command_line = "-triangulationHeuristic one-edge -outputTriangulatedFile $child1"; 
      my @mut_trifile_names = ( $child1 );
      my @mut_timing_option_strings = ( $parent1_vcap );

      push @method_list, {
        'METHOD_NAME'=>'one-edge', 
        'TRI_COMMAND_LINE'=>$tri_command_line, 
        'BASE_TRIANGULATION'=>$parent1_name,
        'TIMING_OPTION_STRINGS'=>\@mut_timing_option_strings,
        'TRIFILE_NAMES'=>\@mut_trifile_names, 
        'TRIANGULATION_MODE'=>$global_triangulation_mode };
    }

  } # foreach boundary 

  #####################################################################
  # Clear old pool 
  #####################################################################
  {
    lock($trifile_pool_in_use);
    @trifile_pool = ();
  }

  make_and_time_trifile_pool( @method_list );


}

####################################################################
# shuffle_array
# 
# Helper function: randomly permutes elements in an array. Ref: Perl
# Cookbook pg. 121: fisher_yates_shuffle
#####################################################################
sub shuffle_array
{
  my $array = shift;
  my $i;
  for ($i = @$array; --$i; ) {
    my $j = int rand ($i+1);
    next if $i == $j;
    @$array[$i,$j] = @$array[$j,$i];
  }
}


#############################################################################
# generate_initial_pool() 
#
# 
#############################################################################
sub generate_initial_pool
{
  my $tri_command_line;
  my $method_name;
  my $trifile_name;
  my $boundary_file;
  my $boundary_index;
  my $boundary_M_type;
  my $boundary_D_type;
  my $bndry_unique;
  my @method_list;
  my @boundary_list;

  $boundary_file = $boundary_queue->dequeue();
  $boundary_index = 0;

  while ($boundary_file ne 'BOUNDARY_SEARCHES_DONE') 
  {
    #####################################################################
    # Check to see if the boundary is unique 
    #####################################################################
    $bndry_unique = 1;
  
    for( my $prvs_bndry = 0; 
         ($prvs_bndry<(scalar @boundary_list)) && ($bndry_unique); 
         $prvs_bndry++)
    {
      $bndry_unique = boundary_files_ne( 
        $boundary_list[$prvs_bndry], $boundary_file );
    }

    if ($bndry_unique)
    {
      print "B$boundary_index ==> Boundary from '$boundary_file'\n";
      push @boundary_list, $boundary_file;
      $boundary_index++; 
    }

    $boundary_file = $boundary_queue->dequeue();
  }

  $numBoundaries = $boundary_index;

  reset_basic_methods();
  ($tri_command_line, $method_name) = get_basic_triangulation_method();
  while ($method_name ne 'DONE')
  {
    $boundary_index = 0;
    foreach $boundary_file (@boundary_list)
    {
      my @trifile_names;
      my @timing_option_strings;

      #####################################################################
      # Generate random timing options (unless completed or copy is used,
      #  then use the default) 
      #####################################################################
      if (($method_name =~ /^completed\./) ||
          ($method_name =~ /^copy\./)) {
        @timing_option_strings = ( $timing_option_list[0] ); 
      }
      elsif ($method_name eq 'crossover') {
        @timing_option_strings = 
          ( $timing_option_list[int(rand(scalar @timing_option_list))],
            $timing_option_list[int(rand(scalar @timing_option_list))] );
      } 
      else {
        @timing_option_strings = 
          ( $timing_option_list[int(rand(scalar @timing_option_list))] );
      }

      #####################################################################
      # Get boundary type from the boundary file name 
      #####################################################################
      ($boundary_M_type) = $boundary_file =~ /([^\/]+)$/;
      ($boundary_M_type, $boundary_D_type) = $boundary_M_type =~ 
        /[^\.]+\.[^\.]+\.(M\d+)\.[^\.]+\.(D_[^\.]+)\./;
      if ((!defined $boundary_M_type) || (!defined $boundary_D_type)) {
        ($boundary_M_type) = $boundary_file =~ /([^\/]+)$/;
        ($boundary_M_type, $boundary_D_type) = $boundary_M_type =~ 
          /B\d+\.(M\d+)\.(D_[^\.]+)\./;
      }

      #####################################################################
      # Add the method to the list 
      #####################################################################
      @trifile_names = ( 
        "$output_directory/B$boundary_index.$boundary_M_type.$boundary_D_type.$method_name.$str_basename.trifile" );
      $tri_command_line = $tri_command_line . 
        " -outputTriangulatedFile $trifile_names[0]"; 

      push @method_list, {
        'TRI_COMMAND_LINE'=>$tri_command_line, 
        'METHOD_NAME'=>$method_name, 
        'BASE_TRIANGULATION'=>$boundary_file, 
        'TIMING_OPTION_STRINGS'=>\@timing_option_strings,
        'TRIFILE_NAMES'=>\@trifile_names, 
        'TRIANGULATION_MODE'=>$global_triangulation_mode };

      $boundary_index++ 
    }

    ($tri_command_line, $method_name) = get_basic_triangulation_method();
  } 
    
  make_and_time_trifile_pool( @method_list );
}

#############################################################################
# generate_pool_from_files() 
#
# 
#############################################################################
sub generate_pool_from_files
{
  my @trifiles = @_;
  my $trifile;

  {
    lock($trifile_pool_in_use);
    @trifile_pool = ();
  }

  foreach $trifile (@trifiles)
  {
    enqueue_trifile($trifile);
  }

  ##########################################################################
  # Wait for all trifiles to be timed
  ##########################################################################
  send_timing_thread_message('BECOME_IDLE'); 
}

################################################################################
#
# GENETIC ALGORITHM SELECTION FUNCTIONS
#
################################################################################

#############################################################################
# trifile_pool_selection 
#
# Main GA selection function. First argument is selection method:
# {'roulette', 'deathmatch','sus'}. Second argument is 'elitist' if
# desired. Third argument is desired population size
#############################################################################
sub trifile_pool_selection
{
    my $select_algo = shift;
    my $elitist = shift;
    my $desired_popsize = shift;
    my $separateGA = shift;
    
    infoMsg(20,"trifile_pool_selection: called with $select_algo $elitist $desired_popsize $separateGA\n");

    lock($trifile_pool_in_use);

    # Get some statistics of the older population (for diagnostics)
    my $maxscore = -2;
    my $bestgene;
    my $totalscore = 0;
    my %trifile_pool_by_boundary = (); # divides up trifile pool into
                                       # separate subsets so to run
                                       # selection separately on
                                       # different boundaries

    for(my $i=0; $i<(scalar @trifile_pool); $i+=2)
      {
	  $trifile_pool[$i+1] += 2;
	  my $trifile_name = $trifile_pool[$i];
	  my $trifile_partitions = $trifile_pool[$i+1];
	  $totalscore += $trifile_partitions;
	  if ($maxscore < $trifile_partitions){
	      $maxscore = $trifile_partitions;
	      $bestgene = $trifile_name;
	  }
	  
	  if ($separateGA){
	      # populate trifile_pool_by_boundary
	      $trifile_name =~ /(B\d+)\./;
	      my $boundary_name = $1;
	      push @{$trifile_pool_by_boundary{$boundary_name}}, $trifile_name;
	      push @{$trifile_pool_by_boundary{$boundary_name}}, $trifile_partitions;
	  }
      }
    
    my $numgene = (scalar(@trifile_pool)/2);
    my $avescore = $totalscore / $numgene;
    infoMsg(20, "Average score: %0.2f   Max score: %f\n", $avescore-2, 
      $maxscore-2);
    infoMsg(30, "Best gene: $bestgene\n");
    infoMsg(20, "\n");

    if ($separateGA){
	## run a selection separately for each boundary
	infoMsg(20,"trifile_pool_selection: run a selection separately for each boundary\n");

	@trifile_pool = ();
	my $desired_subpopsize = int($desired_popsize/$numBoundaries);

	foreach my $k (keys %trifile_pool_by_boundary){
	    infoMsg(20, "Selection for subpool: $k, size: %d\n",
	      scalar(@{$trifile_pool_by_boundary{$k}})/2 ); 

	    my @subpool = ();

	    # Now do specific selection algorithm
	    if ($select_algo eq 'roulette'){
		@subpool = roulette_wheel_selection($desired_subpopsize,@{$trifile_pool_by_boundary{$k}});
	    }
	    elsif ($select_algo eq 'deathmatch'){
		# TODO: Currently the deathmatch parameter is set here. May
		# want it to be set at another place.
		my $tourn_size = 5;
		@subpool = deathmatch_selection($tourn_size,$desired_subpopsize,@{$trifile_pool_by_boundary{$k}});
		#print "new subpool size: ";
		#print scalar(@subpool)/2 . "\n";
		my $i = $#subpool - 1;
		#print "$subpool[0] $subpool[2] $subpool[$i]\n";
	    }
	    elsif ($select_algo eq 'sus'){
		@subpool = stochastic_universal_selection($desired_subpopsize,@{$trifile_pool_by_boundary{$k}});
	    }
	    else {
		die "No such selection algorithm $select_algo implemented! Exiting.\n";
	    }

	    @trifile_pool = (@trifile_pool,@subpool);
	    infoMsg(20, "Pool size after selection: %d\n\n", 
              scalar(@trifile_pool)/2 );
	}
    }
    else {
	## run one selection on total trifile_pool
	infoMsg(20,"trifile_pool_selection: run one selection on total trifile_pool\n");

	# Now do specific selection algorithm
	if ($select_algo eq 'roulette'){
	    @trifile_pool = roulette_wheel_selection($desired_popsize,@trifile_pool);
	}
	elsif ($select_algo eq 'deathmatch'){
	    # TODO: Currently the deathmatch parameter is set here. May
	    # want it to be set at another place.
	    my $tourn_size = 5;
	    @trifile_pool = deathmatch_selection($tourn_size,$desired_popsize,@trifile_pool);
	}
	elsif ($select_algo eq 'sus'){
	    @trifile_pool = stochastic_universal_selection($desired_popsize,@trifile_pool);
	}
	else {
	    die "No such selection algorithm $select_algo implemented! Exiting.\n";
	}
    }

    # if elitism is used, replace random gene in new tri_pool with
    # best gene found previously.
    if ($elitist eq 'elitist'){
	my $randindex = 2 * int(rand(($#trifile_pool/2)));
	$trifile_pool[$randindex] = $bestgene;
	$trifile_pool[$randindex+1] = $maxscore;
	infoMsg(30, "Elitism: Replacing gene of index $randindex with $bestgene, score $maxscore\n\n");
    }
}


#############################################################################
# roulette_wheel_selection 
#
# Samples genes with replacement with probability porpotional to each
# gene's fitness. This code works by mapping each gene's fitness score
# to a line, with the segment of each gene equal to its fitness
# value. Then, random numbers are chosen on this line to select genes.
#############################################################################

sub roulette_wheel_selection 
{

    my $desired_popsize = shift;
    my @pool = @_;
    my $trifile_name;
    my $trifile_partitions;
    my @new_trifile_pool = ();
    my $totalscore = 0; # total length of line
    my @scoreboundary = (); # boundary of each gene's segment

    for(my $i=0; $i<(2*$desired_popsize); $i+=2)
      {
	  my $trifile_name = $pool[$i];
	  my $trifile_partitions = $pool[$i+1];
	  #print "Trifile Name:$trifile_name   Partitions Per Second:$trifile_partitions\n";

	  # create "line"
	  $totalscore += $trifile_partitions;
	  push @scoreboundary, $totalscore;
      }

    # choose N random numbers
    for (my $i=0;$i<$desired_popsize;$i++){

	my $randnum = rand($totalscore);

	# find where this randnum points to on the line
	my $index = 0;
      FOUND: for my $score (@scoreboundary){
	    if ($score >= $randnum){
		last FOUND;
	    }
	    $index++;
	}

	# populate new trifile pool
	my $newtri = $pool[2*$index];
	my $newscore = $pool[2*$index+1];
	push @new_trifile_pool, $newtri;
	push @new_trifile_pool, $newscore;
	#print "New gene: $newtri / Score: $newscore\n";
    }

    return @new_trifile_pool;
}

#############################################################################
# stochastic_universal_selection()
#
# Baker's Stochastic Universal Sampling selection algorithm. This is
# similar to roulette wheel, except the problem of biasing
# toward highly fit genes which emerges in practice is remedied.
#############################################################################

sub stochastic_universal_selection
{

    my $desired_popsize = shift;
    my @pool = @_;
    my $trifile_name;
    my $trifile_partitions;
    my @new_trifile_pool = ();
    my $totalscore = 0; # total length of line
    my @scoreboundary = (); # boundary of each gene's segment

    for(my $i=0; $i<(2*$desired_popsize); $i+=2)
      {
	  my $trifile_name = $pool[$i];
	  my $trifile_partitions = $pool[$i+1];

	  # create "line"
	  $totalscore += $trifile_partitions;
	  push @scoreboundary, $totalscore;
      }

    # choose 1 random number and generate N equally spaced "pointers"
    # to the line
    my $randnum = rand($totalscore);
    my $interval = $totalscore/$desired_popsize;
    my @pointers = ();
    for (0..($desired_popsize-1)){
	my $pointer = ($interval*$_ + $randnum) % $totalscore;
	push @pointers, $pointer;
    }
    @pointers = sort {$a <=> $b} @pointers;

    # find out where the N pointers point to on the line
    for my $pointer (@pointers){

	my $index = 0;

	# find where this pointer points to on the line
	FOUND: for my $score (@scoreboundary){
	    if ($score >= $pointer){
		last FOUND;
	    }
	    $index++;
	}

	# populate new trifile pool
	my $newtri = $pool[2*$index];
	my $newscore = $pool[2*$index+1];
	push @new_trifile_pool, $newtri;
	push @new_trifile_pool, $newscore;
	#print "New gene: $newtri / Score: $newscore\n";
    }

    return @new_trifile_pool;
}


#############################################################################
# deathmatch_selection(tourn_size)
#
# A GA selection algorithm more commonly known as "tournament
# selection". Selection pressure can be changed by adjusting the
# tournament size 'tourn_size'.  If N is large, lesser fit genes have
# lower chance of surviving due to more competition. Current
# implementation is called "deterministic tournament selection" since
# the strongest gene in a tournament always wins.
#############################################################################
sub deathmatch_selection
{
    my $tourn_size = shift;
    my $desired_popsize = shift;
    my @pool = @_;
    my $current_best_gene;
    my $current_best_score;

    infoMsg(30, "deathmatch champions: ");
    # Hold many independent tournaments to populate new trifile_pool
    my @new_trifile_pool = ();
    my $total_population = scalar(@pool) / 2;
    for my $i (0..($desired_popsize-1)){

        do {
	    # randomly select $tourn_size genes for current deathmatch
	    $current_best_gene = '';
	    $current_best_score = -2;
	    
	    for my $j (0..($tourn_size-1)) {
		my $randindex = 2*int(rand($total_population));
		my $score = $pool[$randindex+1];
		
		if ($current_best_score < $score){
		    $current_best_score = $score;
		    $current_best_gene = $pool[$randindex];
		}
	    }
        } while($current_best_score < 0);

	$new_trifile_pool[2*$i] = $current_best_gene;
	$new_trifile_pool[2*$i+1] = $current_best_score;
	infoMsg(30, "$current_best_score ");
    }
    infoMsg(30, "\n");

    # replace old gene pool
    return @new_trifile_pool;
}

################################################################################
#
################################################################################

#############################################################################
# make_and_time_trifile_pool 
#
# Create trifiles for each method given in the list.  If parallelism is >1 this
# is done asynchronously.  After the trifile has been created is placed in the
# queue to be timed.
#
# PARAMETERS:
#   @method_list        - List of references to hashes containing 'METHOD'
#                         'METHOD_NAME', and 'BASE_TRIANGULATION'
#   $triangulation_mode - String defining a 'P', 'C', or 'E' triangulation 
#############################################################################
sub make_and_time_trifile_pool
{
  my $base_triangulation;
  my @timing_option_strings;
  my $timing_option_string;
  my @trifile_names;
  my $trifile_name;
  my $method_hash;
  my $thread_id;
  my $child;
  my @method_list;
  my @thread_list;
  my $i;

  @method_list = @_;

  #########################################################################
  # For each method given, create a thread to make and time the trifile  
  #########################################################################
  foreach $method_hash (@method_list)
  {
    my @trifile_names;

    if ($method_hash->{'METHOD_NAME'} =~ /^copy\./)
    {
      $base_triangulation = $method_hash->{'BASE_TRIANGULATION'};
      @timing_option_strings = @{$method_hash->{'TIMING_OPTION_STRINGS'}};
      @trifile_names = @{$method_hash->{'TRIFILE_NAMES'}};

      for ($i=0; $i<(scalar @trifile_names); $i++)
      {
        copy($base_triangulation, $trifile_names[$i]);
        enqueue_trifile("${trifile_names[$i]}____${timing_option_strings[$i]}");
      }
    }
    else {
      $thread_id = make_and_time_trifile( %{$method_hash} ); 
      push @thread_list, $thread_id;
    }

  }

  #########################################################################
  # Wait for the triangulation threads to complete
  #########################################################################
  infoMsg(20,"Waiting for triangulation threads...\n");
  foreach $child (@thread_list)
  {
    if ($child != 0) {
      $child->join;
    }
  }
  infoMsg(20,"...joined\n");

  send_timing_thread_message('BECOME_IDLE'); 
}

################################################################################
#
# TRIANGULATION FUNCTIONS
#
# make_and_time_trifile 
#    Creates triangulation and times it when done (possibly asynchronously).  
#
# make_trifile
#    Make trifile but do not time it (possibly asynchronous).  
#
# make_trifile_thread
#    Function controlling thread that Calls run_triangulation, handles any 
#    errors.  When the triangulation has been successfully created the trifile 
#    is put on the queue to be timed. 
#
# run_triangulation
#    Calls gmtkTriangulate and returns the jtWeight of the resulting 
#    triangulation.
#
################################################################################

#############################################################################
# make_and_time_trifile 
#
# Create trifile with given method.  If parallelism is >1 this is done 
# ansynchrounsly.  After the trifile has been created is placed in the queu
# to be timed.
#
# PARAMETERS:
#   $boundary_name      - String identifying the boundary, such as 'B1'   
#   $boundary           - Name of trifile containing partition information 
#   $method_options     - String containing gmtkTriangulate command line 
#                         parameters
#   $triangulation_mode - String defining a 'P', 'C', or 'E' triangulation 
#   $trifile_name       - Name of the trifile
#############################################################################
sub make_and_time_trifile
{
  my %parameters = @_;
  my $thread_id;

  $needed_trifiles->down(scalar @{$parameters{TRIFILE_NAMES}});

  if ($parallelism == 1) {
    make_trifile_thread( %parameters ); 
    $thread_id = 0;
  }
  else {
    $thread_id = threads->new( \&make_trifile_thread, %parameters );
  }

  return($thread_id);
}


#############################################################################
# make_trifile_thread 
#
# Calls run_triangulation and handles any errors.  When the triangulation 
# has been successfully created the trifile is put on the queue to be timed. 
#
# PARAMETERS:
#   $boundary_name      - String identifying the boundary, such as 'B1'   
#   $boundary           - Name of trifile containing partition information 
#   $method_options     - String containing gmtkTriangulate command line 
#                         parameters
#   $triangulation_mode - String defining a 'P', 'C', or 'E' triangulation 
#############################################################################
sub make_trifile_thread
{
  my %parameters = @_;
  my $file_h;
  my $real_trifile_name;
  my $triangulation_status;
  my @trifile_names;
  my @timing_option_strings;

  do {
    $triangulation_status = run_triangulation(
      TRI_COMMAND_LINE=>$parameters{'TRI_COMMAND_LINE'}, 
      BASE_TRIANGULATION=>$parameters{'BASE_TRIANGULATION'}, 
      TRIANGULATION_MODE=>$parameters{'TRIANGULATION_MODE'}
      );

    if ($triangulation_status < 0) {
      print "Triangulation failed\n";
      sleep(15);
    } 
  } while($triangulation_status< 0);
  
  infoMsg(20,"make_trifile thread: run_triangulation completed\n");

  @trifile_names = @{$parameters{'TRIFILE_NAMES'}};

  @timing_option_strings = @{$parameters{'TIMING_OPTION_STRINGS'}};
  for(my $i=0; $i<(scalar @trifile_names); $i++)
  {
    enqueue_trifile("${trifile_names[$i]}____${timing_option_strings[$i]}");
  }

  infoMsg(20,"make_trifile thread: thread completed\n");
}


#############################################################################
# run_triangulation( method );
#
# Calls gmtkTriangulate and returns the jtWeight of the resulting 
# triangulation.
#
# PARAMETERS:
#   $method_options     - String containing the command line parameters which
#                         define the triangulation heuristic and possibly
#                         other options. 
#   $boundary           - Input trifile which contains the partition 
#                         information and triangulations of the partitions 
#                         which are not being changed.
#   $output_tri_file    - Filename of resulting triangulation
#   $triangulation_mode - Set to 'P' for prologue, 'C' for chunk, and 'E' 
#                         for epilogue
#############################################################################
sub run_triangulation
{
  my %parameters = @_;
  my $triangulation_mode = $parameters{'TRIANGULATION_MODE'};
  my $base_triangulation = $parameters{'BASE_TRIANGULATION'};
  my $tri_command_line   = $parameters{'TRI_COMMAND_LINE'};

  my $output;
  my $jt_weight;
  my $real_time;
  my $triangulate_call;
  my $triangulation_mode_string;
  my $bndry_M;
  my $disconnect_type;

  if ($triangulation_mode eq 'P') {
    $triangulation_mode_string = '-noReTriP F -noReTriC T -noReTriE T'; 
  }
  elsif ($triangulation_mode eq 'C') {
    $triangulation_mode_string = '-noReTriP T -noReTriC F -noReTriE T'; 
  }
  elsif ($triangulation_mode eq 'E') {
    $triangulation_mode_string = '-noReTriP T -noReTriC T -noReTriE F'; 
  }
  else {
    die "***INTERNAL ERROR, invalid triangulation_mode"; 
  }

  my $line_nmbr = `cat -n $base_triangulation | egrep 'M, number of chunks' | awk '{ print \$1 }'`;
  $line_nmbr++; 
  $bndry_M = `head -$line_nmbr $base_triangulation | tail -1`;

  chomp($bndry_M);
  (defined $bndry_M) or die "Can't find M in boundary $base_triangulation";

  ($disconnect_type) = $base_triangulation =~ /([^\/]+)$/;
  ($disconnect_type) = $disconnect_type =~ /^B\d+\.M\d+\.D_(T|F)\./;
  if (!defined $disconnect_type) 
  {
    ($disconnect_type) = $base_triangulation =~ /([^\/]+)$/;
    ($disconnect_type) = $disconnect_type =~ /[^\.]+\..\.M\d+\.UB_.\.D_(T|F)\./;
  }

  do {
    $triangulate_call = "$triangulation_export_line /usr/bin/time $gmtk_triangulate -strFile $str_file -rePartition F -reTriangulate T -findBestBoundary F -inputTriangulatedFile $base_triangulation -seed T -printResults T -numBackupFiles 0 $triangulation_mode_string -jtWeight T -M $bndry_M -disconnectFromObservedParent $disconnect_type $tri_command_line 2>&1";
    #print "$triangulate_call\n";

    $output = `$triangulate_call`;

    print STDERR "$output\n";

    ($real_time) = $output =~ /user\s+([0-9.]+)/;
    if (!defined $real_time)
    {
      ($real_time) = $output =~ /([0-9.]+)user /;
    }

  } while ( !defined $real_time);

  ($jt_weight) = $output =~ /Chunk max clique weight =.* jt_weight = ([0-9.]+)/;

  if (!defined $jt_weight) {
    $jt_weight = 0; 
  }
  else {
    $jt_weight = 1; 
  }

  record_time($real_time);
 
  return($jt_weight); 
}

################################################################################
# TIMING FUNTIONS
#
# enqueue_trifile  
#    Call this to place a trifile on the queue to be timed 
#
# time_triangulation_thread 
#    Main control function of thread that pulls trifiles from the queue and 
#    times them
#
# dequeue_and_time_triangulation 
#    Removes a trifile from the queue, interprets commands, and calls 
#    run_timing
#
# run_timing
#    Called to do the actual work of communicating with the external timing 
#    process 
#
# open_time
#    Opens a pipe to an instance of gmtkTime in multiTest mode 
#
################################################################################

#############################################################################
# enqueue_trifile 
#
# This function is called to indicate that a trifile is ready to be timed.  
# If parallelism is 1, the trifile is timed immediately.
#
# PARAMETERS:
#   $trifile - The name of the trifile
#############################################################################
sub enqueue_trifile 
{
  my $trifile = pop;
  my $exit;

  $trifile_queue->enqueue($trifile);
  
  if ($parallelism == 1) {
    ( $single_processor_timing_script_input, 
      $single_processor_timing_script_output, 
      $single_processor_current_pid, 
      $exit ) = 
      dequeue_and_time_triangulation( 
      $single_processor_timing_script_input, 
      $single_processor_timing_script_output, 
      $single_processor_current_pid); 
  }
}

#############################################################################
# time_triangulation_thread() 
#
# There is an instance of this thread for each copy of gmtkTime (except when
# parallelism==1 no instances of this thread run).
#############################################################################
sub time_triangulation_thread 
{
  my $timing_script_input;  
  my $timing_script_output;
  my $current_pid;
  my $exit;

  #####################################################################
  # Open a copy of gmtkTime in multiTest mode 
  #####################################################################
  $timing_script_input  = gensym;
  $timing_script_output = gensym;

  ($current_pid, $timing_script_input, $timing_script_output) = open_time();

  #####################################################################
  # Generate and time triangulations 
  #####################################################################
  while (!$exit) 
  {
    infoMsg(25,"time_triangulation_thread: current_pid=$current_pid\n");
    ($timing_script_input, $timing_script_output, $current_pid, $exit) = 
      dequeue_and_time_triangulation( 
      $timing_script_input, $timing_script_output, $current_pid ); 
  } 

}

#############################################################################
# send_timing_thread_message()
#
# Sends a command and control message to the timing threads 
#
# PARAMETERS:
#  $message - string containing the message code
#############################################################################
sub send_timing_thread_message 
{
  my $message = pop; 
  infoMsg(20,"send_timing_thread_message: sending $message\n");

  #########################################################################
  # Send message to each thread 
  #########################################################################
  infoMsg(25,"send_timing_thread_message: $nmbr_timing_processes timing processes expected\n");
  foreach my $thr (threads->list){
     infoMsg(25,"send_timing_thread_message: $thr\n");
  }
  for (my $i=0; $i<$nmbr_timing_processes ; $i++) {
    infoMsg(20,"send_timing_thread_message: enqueuing $message\n");
    enqueue_trifile($message);
  }

  #########################################################################
  # Wait for all threads to acknowledge that they got the message 
  #########################################################################
  infoMsg(20,"send_timing_thread_message: waiting for threads to acknowledge\n");
  $timing_thread_idle_sem->down($nmbr_timing_processes);
  sleep(2);
  infoMsg(20,"send_timing_thread_message: downed the timing_thread_idle_sem semaphore $nmbr_timing_processes times.\n");

  #########################################################################
  # Tell the timing threads that they can continue 
  #########################################################################
  lock($timing_thread_idle);
  cond_broadcast($timing_thread_idle);
  infoMsg(20,"send_timing_thread_message: sent $message\n");
}


#############################################################################
# dequeue_and_time_triangulation() 
#
# Removes a item from the trifile queue.  Process all items in the front of 
# the queue which are commands.  Then time the trifile.   
#
# PARAMETERS:
#  $timing_script_input, $timing_script_output, $crrnt_pid, $vcap, $cache  -
#    details of the gmtkTime process in case it needs to be restarted.     
#############################################################################
sub dequeue_and_time_triangulation 
{
  my $crrnt_pid;
  my $timing_script_input;
  my $timing_script_output;
  my $processing_command;
  my $crrnt_partitions;
  my $real_time;
  my $boundary;
  my $method;
  my $exit = 0;
  my $dequeue_value;
  my $trifile_name;
  my $caching;
  my $vcap;

  $crrnt_pid = pop;
  $timing_script_output = pop;
  $timing_script_input  = pop;
  infoMsg(20,"dequeue_and_time_triangulation: called with $crrnt_pid,...\n");

  #####################################################################
  # Get the next trifile from the queue 
  #####################################################################
  $needed_trifiles->up;

  do 
  {
    $processing_command = 0;
    $dequeue_value = $trifile_queue->dequeue;
    infoMsg(25,"dequeue_and_time_triangulation: dequeue_value=$dequeue_value\n");

    if ($dequeue_value eq 'BECOME_IDLE') 
    {
      $processing_command = 1;
    }
    elsif ($dequeue_value eq 'REBOOT')
    {
      printf "CLOSING %x\n", threads->self;
      print STDERR "CLOSING\n";

      close $timing_script_input;
      close $timing_script_output;
      waitpid $crrnt_pid, 0;

      ($crrnt_pid, $timing_script_input, $timing_script_output) = open_time();

      $processing_command = 1;
      printf "DONE CLOSING %x\n", threads->self;
    }
    elsif ($dequeue_value eq 'EXIT') 
    {
      $exit = 1;
      $timing_thread_idle_sem->up;
      lock($timing_thread_idle);
      cond_wait($timing_thread_idle);
      infoMsg(20,"dequeue_and_time_triangulation: EXIT acknowledged\n");
    }
    else 
    {
      ($trifile_name, $vcap, $caching) = $dequeue_value =~
        /(.*)____(.*)$/;
    }

    infoMsg(20,"dequeue_and_time_triangulation: processing_command is $processing_command\n");

    if ($processing_command) 
    {
      $timing_thread_idle_sem->up;
      infoMsg(20,"dequeue_and_time_triangulation: upped the timing_thread_idle_sem semaphore.\n");
      lock($timing_thread_idle);
      cond_wait($timing_thread_idle);
      infoMsg(20,"dequeue_and_time_triangulation: acknowledged\n");
    }

  } while ($processing_command); 

  #####################################################################
  # Time the trifile 
  #####################################################################
  if (!$exit) {

    ($crrnt_pid, $timing_script_input, $timing_script_output, 
    $crrnt_partitions, $real_time) = run_timing( $trifile_name, $crrnt_pid, 
    $timing_script_input, $timing_script_output, 
    "vcap=\"$vcap\"" );

    add_to_trifile_pool($dequeue_value, $crrnt_partitions);
    check_best($crrnt_partitions, $real_time, $vcap, $caching, $trifile_name);
  }
  infoMsg(20,"dequeue_and_time_triangulation: finished dequeueing\n");
  infoMsg(20,"dequeue_and_time_triangulation: returning with ... , ... , $crrnt_pid, $exit\n");

  return($timing_script_input, $timing_script_output, $crrnt_pid, $exit);
}

#############################################################################
# run_timing 
#
# Sends a trifile name to the stdin of a gmtkTime process.  It then waits
# for the timing to complete and parses the results.
#
# PARAMETERS:
#    $tri_file - Name of the trifile to be timed
#    $crrnt_pid, $timing_script_input, $timing_script_output, $timing_options,
#      - details of the gmtkTime process in case it needs to be restarted.     
#############################################################################
sub run_timing 
{
  my $timing_options;
  my $tri_file;
  my $output;
  my $partitions;
  my $real_time;

  my $crrnt_pid; 
  my $timing_script_input; 
  my $timing_script_output; 
  my $error_found; 
  my $run_timing_deadline;

  $timing_options = pop;
  $timing_script_output = pop; 
  $timing_script_input  = pop; 
  $crrnt_pid = pop; 
  $tri_file  = pop;

  infoMsg(20,"run_timing:called with $timing_options, $crrnt_pid, $tri_file\n");

  do { 
    $run_timing_deadline = time + 200;
    infoMsg(20,"run_timing:set deadline to $run_timing_deadline\n");
    $error_found = 0;
    eval {
      print $timing_script_input "trifile=\"$tri_file\" $timing_options \n";
    
      ##########################################################################
      # Read in pre timing output:
      #   cpp warnings ....        
      #   exported to ....        # not needed if not exporting  
      #   --------
      #   0: Operating on trifile ''
      #   0: Running program for approximately 7 seconds
      ##########################################################################
      do {
        $output = <$timing_script_output>;
        (defined $output) or die;
        print STDERR "$output";
        chomp($output);
      } while($output ne '--------');
    
      $output = <$timing_script_output>;
      (defined $output) or die;
      print STDERR "$output";
      $output = <$timing_script_output>;
      (defined $output) or die;
      print STDERR "$output";
      $output = <$timing_script_output>;
      (defined $output) or die;
      print STDERR "$output";
    };
   
    ###########################################################################
    # An error at this point means that the trifile can't be sent to gmtkTime,
    # so restart the process
    ###########################################################################
    if ($@) {
      waitpid $crrnt_pid, 0; 
      ($crrnt_pid, $timing_script_input, $timing_script_output) = open_time( 
        $timing_options );
      $error_found = 1;
    } 

  } while($error_found);
  infoMsg(20,"run_timing:have read in pre timing output\n");
 
  ###########################################################################
  # Read in timing results:
  #   User: 6.980000, System: 0.030000, CPU 7.010000
  #   0: Inference stats: 6.98...
  ###########################################################################
  eval {

    do {
      if(time > $run_timing_deadline){
         infoMsg(20,"run_timing:deadline of $run_timing_deadline not met\n");
         $real_time = $INFINITY;
      }else{
         $output = <$timing_script_output>;
         (defined $output) or die;
         print STDERR "$output";
         ($real_time)  = $output =~ /User: ([0-9.]+)/;;
      }
    } while (!defined $real_time); 
    infoMsg(25,"run_timing:real_time loop finished\n");

    if (!($output =~ /NOTICE/)) {
      $output = <$timing_script_output>;
      (defined $output) or die;
      print STDERR "$output";
    }

    infoMsg(25,"run_timing:spurious not NOTICE line done\n");

    ($partitions) = $output =~ /, ([0-9+-.e]+) partitions\/sec/;
  };

  (defined $partitions) or $partitions = -1; 

  infoMsg(20,"run_timing: returning with $crrnt_pid, ... , ... , $partitions, $real_time\n");
  return($crrnt_pid, $timing_script_input, $timing_script_output, $partitions, $real_time );
}


#############################################################################
# open_time
#
# Opens an instance of gmtkTime using the $timing_export_line.
# 
# PARAMETERS:
#   none 
#############################################################################
sub open_time
{
  my $options;
  my $error;
  my $output;
  my $pid;
  my $timing_script_input;
  my $timing_script_output;
  my $timing_command_line;
  my $extra_options;

  do {
    $error = 0;
    eval 
    {

      if ($global_triangulation_mode eq 'P') {
        $extra_options = '-gpr 0:6 -noEPartition T';
      }
      elsif ($global_triangulation_mode eq 'C') {
        $extra_options = '-noEPartition T';
      }
      elsif ($global_triangulation_mode eq 'E') {
        $extra_options = '-gpr 0:20';
      }

      $timing_command_line = "$timing_export_line $timing_script -strFile $str_file -seconds $timing_length -multiTest T $extra_options 2>&1 ";
      print STDERR "OPENING:$timing_command_line\n"; 

      $pid = open2( $timing_script_output, $timing_script_input, 
        $timing_command_line );

      $timing_script_input->autoflush(1);
      $output = <$timing_script_output>;
      print STDERR "$output";
    };

    if ($@ || ($output=~'no suitable host available')) 
    { 
      $error = 1;
      chomp($output);
      if ( !$boundary_searches_done ) {
        sleep(1);
      }
      else {
        print "Problem opening timing script:'$output'\n";
      }
      waitpid $pid, 0; 
      sleep(1);
    }

  } while ($error);

  return ($pid, $timing_script_input, $timing_script_output);
}

################################################################################
#
# RESULT FUNCTIONS
#
################################################################################

#############################################################################
# Record time 
#############################################################################
sub record_time : locked 
{
  $cumulative_time += pop; 
}

#############################################################################
# add_to_trifile_pool 
#
# Does a thread safe addition of a trifile to the global trifile pool 
#
# PARAMETERS:
#   $partitions_sec  - Partitions per second achieved by the trifile 
#   $trifile_name    - Name of trifile
#############################################################################
sub add_to_trifile_pool 
{
  my $partitions_sec = pop;
  my $trifile_name = pop;

  lock($trifile_pool_in_use);

  push @trifile_pool, ( $trifile_name, $partitions_sec ); 
}

#############################################################################
# check_best 
#
# Determines if a timing is the best overall triangulation and if it is the
# best for its boundary.
#
# PARAMETERS:
#   $nmbr_partitions - number of partitions finished when timing 
#   $vcap            - gmtkTime option
#   $caching         - gmtkTime option
#   $trifile_name    - Name of trifile
#   $real_time       - 
#############################################################################
sub check_best 
{
  my $method;
  my $boundary;
  my $boundary_index;
  my $tmp;
  my $nmbr_partitions;
  my $real_time;
  my $vcap;
  my $caching;
  my $trifile_name;
  my $overall_best_flag;
  my $crrnt_time;

  $trifile_name    = pop;
  $caching         = pop;
  $vcap            = pop;
  $real_time       = pop;
  $nmbr_partitions = pop;

  record_time($real_time);

  $best_sem->down;

  ($method) = $trifile_name =~ /(input\.\d+\.[^\/]+)$/; 
  if (! defined $method) {
    ($method) = $trifile_name =~ /B[0-9PE]+.([^\/]+)\.$str_basename/;
  }

  ($tmp, $boundary_index) = $trifile_name =~ /(^|\/)B([0-9PE]+)\./;
  $tmp = '';
  if (!defined $boundary_index) {
    ($boundary_index) = $method =~ /input\.(\d+)\./;
  }

  $boundary = "B$boundary_index";

  $crrnt_time = time() - $start_time;

  #    int($cumulative_time+0.5), 
  printf "%-8d%-3s%-39s%-5s %4e", $crrnt_time, 
    $boundary, $method, $vcap, 
    $nmbr_partitions;

  if ($nmbr_partitions > $best_partitions) {
    print " => BEST!!!";
    $best_partitions = $nmbr_partitions;
    $best_vcap = $vcap;
    $best_boundary_index = $boundary_index;
    $best_trifile = $trifile_name; 
    copy( $trifile_name, "$str_basename.best.trifile"); 
    $overall_best_flag = 1;
  }
  else {
    $overall_best_flag = 0;
  }

  if (($boundary_index ne 'P') && 
      ($boundary_index ne 'E') && 
      ($nmbr_partitions > $best_time_per_boundary[$boundary_index]))
  {
    if (!$overall_best_flag) {
      print " => bndry. best";
    }
  
    $best_time_per_boundary[$boundary_index] = $nmbr_partitions;
  }
 
  print "\n";

  $best_sem->up;
}

################################################################################
#
# BOUNDARY FUNCTIONS
#
################################################################################

#############################################################################
# get_existing_boundaries 
#
# Reads in the boundary triangulations from the output directory and puts 
# them in the boundary queue.  This function is used when boundary searches
# are not run.
#############################################################################
sub get_existing_boundaries
{
  my @boundary_list;
  my $boundary;
  my $total_boundaries;

  opendir WORK_DIR, "$output_directory" or die 
    "***Error: Can't open triangulation work directory: $output_directory";
  @boundary_list = grep /.*\.boundary$/, readdir WORK_DIR; 
  closedir WORK_DIR;

  ((scalar @boundary_list) > 0) or die "${error_message}No boundary files found\n";

  ###########################################################################
  # Sort small M before large M 
  ###########################################################################
  @boundary_list = sort { 
    my $a_M;
    my $b_M;
    ($a_M) = $a =~ /\.M(\d+)\./;  
    ($b_M) = $b =~ /\.M(\d+)\./;  
    $a_M <=> $b_M;
  } @boundary_list;

  ###########################################################################
  # Enqueue all boundaries 
  ###########################################################################
  $total_boundaries = 0; 
  foreach $boundary (@boundary_list)
  {
    $boundary_queue->enqueue("$output_directory/$boundary");
    $total_boundaries++; 
  }
  
  printf "%d boundary files found\n", $total_boundaries; 

  $boundary_queue->enqueue( 'BOUNDARY_SEARCHES_DONE' ); 
}


#############################################################################
# start_boundary_searches 
#
# Calls GMTK to perform boundary searches, places boundary files in the 
# queue as the searches complete.  
#############################################################################
sub start_boundary_searches 
{
  my $boundary_heuristic;
  my $boundary_job_list;
  my $initial_job_list;
  my $merge_job_list;
  my $boundary_line;
  my $triangulate_line;
  my @weight_UB_types;
  my $weight_UB_type;
  my $left_right;
  my $output_name;
  my $arguments;
  my $boundary_calls;
  my $total_boundaries;
  my $disconnect_type;
  my $boundary_M;
  my @boundary_heuristics; 
  my @no_M_boundary_heuristics; 
  my $maximum_M;
  my $use_UB_weight;
  my $boundary_search_time;
  my $boundary_triangulation_time;


  if ($short_mode) 
  {
    @boundary_heuristics = ('N', 'S');
    $maximum_M = 2;
    @no_M_boundary_heuristics = ('W' );
    $use_UB_weight = 0;
  }
  elsif ($medium_mode) 
  {
    @boundary_heuristics = ('N', 'S');
    $maximum_M = 2;
    @no_M_boundary_heuristics = ('W', 'F', 'Q', 'C', 'A'); 
    $use_UB_weight = 1;
  }
  else
  {
    @boundary_heuristics = ('N', 'S');
    $maximum_M = 4;
    @no_M_boundary_heuristics = ('W', 'F', 'Q', 'C', 'A', 'FWH', 'T', 'P',
      'ST', 'SF', 'SW', 'SFW', 'TS', 'TF', 'TW', 'TSW', 'FS', 'FT', 'FW',
      'FTSW');
    $use_UB_weight = 1;
  }

  print "Starting boundary searches\n";

  ############################################################################
  # Create makefile for boundaries  
  ############################################################################
  $boundary_search_time = (3*get_seconds($boundary_time))/4;
  if ($boundary_search_time < 1) {
    $boundary_search_time = 1;
  }
  $boundary_triangulation_time = get_seconds($boundary_time)/12;
  if ($boundary_triangulation_time  < 1) {
    $boundary_triangulation_time =1;  
  }

  $boundary_job_list = "$output_directory/boundary_job_list";  
  $initial_job_list = "$output_directory/initial_tri_job_list";  
  $merge_job_list = "$output_directory/merge_job_list";  

  $boundary_line = "-strFile $str_file -rePartition T -findBestBoundary T -seed T -printResults F -numBackupFiles 0 -triangulationHeuristic completed -noBoundaryMemoize T -jtWeight T -timeLimit $boundary_search_time";

  $triangulate_line = "-strFile $str_file -rePartition F -reTriangulate T -seed T -printResults F -numBackupFiles 0 -triangulationHeuristic heuristics -jtWeight T -timeLimit $boundary_triangulation_time";

  open BOUNDARY_JOB_LIST, ">$boundary_job_list";
  open TRI_JOB_LIST, ">$initial_job_list";
  open MERGE_JOB_LIST, ">$merge_job_list";

  if(-e "$output_directory/output_names.txt"){
    unlink("$output_directory/output_names.txt") or die "Couldn't delete output_names.txt\n";
  }

  $boundary_calls = 0;

  $boundary_M=1;
  #  foreach $disconnect_type ('T', 'F')
  foreach $disconnect_type ('T')
  {
    foreach $boundary_heuristic (@no_M_boundary_heuristics)
    {
      if (($boundary_heuristic =~ /W/) && ($use_UB_weight)) {
        @weight_UB_types = ('F', 'T');
      }
      else {
        @weight_UB_types = ('F');
      }

      foreach $weight_UB_type (@weight_UB_types)
      {
        foreach $left_right ('L','R')
        {
          if ((!defined $maximum_boundary_searches) || 
              ($boundary_calls < $maximum_boundary_searches))
          { 
            $output_name = "$output_directory/$boundary_heuristic.$left_right.M$boundary_M.UB_$weight_UB_type.D_$disconnect_type.$str_basename.boundary";
	    $arguments = "$boundary_line -outputTriangulatedFile $output_name.B -boundaryHeuristic $boundary_heuristic -jtwUB $weight_UB_type -forceLeftRight $left_right -disconnectFromObservedParent $disconnect_type -M $boundary_M";
	    print BOUNDARY_JOB_LIST "$boundary_export_line $gmtk_triangulate $arguments\n";
            $arguments = "$triangulate_line -inputTriangulatedFile $output_name.B -outputTriangulatedFile $output_name.P -disconnectFromObservedParent $disconnect_type -M $boundary_M -noReTriP F -noReTriC T -noReTriE T";
	    print TRI_JOB_LIST "$boundary_export_line $gmtk_triangulate $arguments\n";

            $arguments = "$triangulate_line -inputTriangulatedFile $output_name.B -outputTriangulatedFile $output_name.C -disconnectFromObservedParent $disconnect_type -M $boundary_M -noReTriP T -noReTriC F -noReTriE T";
	    print TRI_JOB_LIST "$boundary_export_line $gmtk_triangulate $arguments\n";

            $arguments = "$triangulate_line -inputTriangulatedFile $output_name.B -outputTriangulatedFile $output_name.E -disconnectFromObservedParent $disconnect_type -M $boundary_M -noReTriP T -noReTriC T -noReTriE F";
	    print TRI_JOB_LIST "$boundary_export_line $gmtk_triangulate $arguments\n";

            print MERGE_JOB_LIST "$boundary_export_line $gmtk_tfmerge -strFile $str_file -Ptrifile $output_name.P -Ctrifile $output_name.C -Etrifile $output_name.E -numBackupFiles 0 -outputTriangulatedFile $output_name\n"; 
            #if you can come up with a built-in way to append a word to a file without using redirection and thread-safe, replace this!
            print MERGE_JOB_LIST "$SCRIPTSDIR/append $output_name $output_directory/output_names.txt\n";

            $boundary_calls++;
          }
        }
      }
    } 
  } 

  $weight_UB_type = 'F'; 
  for ($boundary_M=1; $boundary_M<=$maximum_M; $boundary_M++)
  {
    #    foreach $disconnect_type ('T', 'F')
    foreach $disconnect_type ('T')
    {
      foreach $boundary_heuristic (@boundary_heuristics)
      {
        foreach $left_right ('L', 'R')
        {
          if ((!defined $maximum_boundary_searches) || 
              ($boundary_calls < $maximum_boundary_searches))
          { 
            $output_name = "$output_directory/$boundary_heuristic.$left_right.M$boundary_M.UB_$weight_UB_type.D_$disconnect_type.$str_basename.boundary";

            $arguments = "$boundary_line -outputTriangulatedFile $output_name.B -boundaryHeuristic $boundary_heuristic -jtwUB $weight_UB_type -forceLeftRight $left_right -disconnectFromObservedParent $disconnect_type -M $boundary_M";
	    print BOUNDARY_JOB_LIST "$boundary_export_line $gmtk_triangulate $arguments\n";
 
            $arguments = "$triangulate_line -inputTriangulatedFile $output_name.B -outputTriangulatedFile $output_name.P -disconnectFromObservedParent $disconnect_type -M $boundary_M -noReTriP F -noReTriC T -noReTriE T";
	    print TRI_JOB_LIST "$boundary_export_line $gmtk_triangulate $arguments\n";
            $arguments = "$triangulate_line -inputTriangulatedFile $output_name.B -outputTriangulatedFile $output_name.C -disconnectFromObservedParent $disconnect_type -M $boundary_M -noReTriP T -noReTriC F -noReTriE T";
	    print TRI_JOB_LIST "$boundary_export_line $gmtk_triangulate $arguments\n";
            $arguments = "$triangulate_line -inputTriangulatedFile $output_name.B -outputTriangulatedFile $output_name.E -disconnectFromObservedParent $disconnect_type -M $boundary_M -noReTriP T -noReTriC T -noReTriE F";
	    print TRI_JOB_LIST "$boundary_export_line $gmtk_triangulate $arguments\n";

            print MERGE_JOB_LIST "$boundary_export_line $gmtk_tfmerge -strFile $str_file -Ptrifile $output_name.P -Ctrifile $output_name.C -Etrifile $output_name.E -numBackupFiles 0 -outputTriangulatedFile $output_name\n";
            #if you can come up with a built-in way to append a word to a file without using redirection and thread-safe, replace this!
            print MERGE_JOB_LIST "$SCRIPTSDIR/append $output_name $output_directory/output_names.txt\n";

            $boundary_calls++;
          }
        }
      }
    } 
  } 

  close BOUNDARY_JOB_LIST; 
  close TRI_JOB_LIST; 
  close MERGE_JOB_LIST; 

  ############################################################################
  # Start searches and enque the boundaries as they finish
  ############################################################################
  print "--------- Starting boundary searches ---------\n";
  run_file($boundary_job_list);

  print "--------- Generating preliminary triangulations ---------\n";
  run_file($initial_job_list);
  run_file($merge_job_list);
  open RUN_JOBS, "$output_directory/output_names.txt";

  $total_boundaries = 0; 
  while (<RUN_JOBS>) 
  {
    print STDERR "$_";

    chomp;
    $output_name = $_;

    $boundary_queue->enqueue($output_name);
    $total_boundaries++; 
  }

  close RUN_JOBS;

  if ($total_boundaries>1)
  {
    printf "%d boundary searches completed\n", $total_boundaries; 
  }
  else 
  {
    print "Boundary search completed\n";
  }

  print `date`;
  $boundary_queue->enqueue( 'BOUNDARY_SEARCHES_DONE' ); 
}


sub run_file{
  my $job_list = pop;

  #loop through
  my @jobs;
  open JOB_LIST,"$job_list";
  while(<JOB_LIST>){
    #run each line
    push @jobs, threads->new(\&run_line,$_);
    sleep $throttling;
  }
  close JOB_LIST;

  print "about to join\n";
  #and then wait for each job
  foreach my $job (@jobs){
    $job->join;
  }
  print "joined\n";
}

sub run_line{
  system($_) == 0 or die "Couldn't run $_\nerrno: $?\nerror message: $@\n";
}

#############################################################################
# boundary_files_ne 
#
# Compares two trifiles and returns true if they use the same boundary, false
# if they do not
#
# $trifile_1, $trifile_2 - File names of trifiles to be compared 
#############################################################################
sub boundary_files_ne
{
  my $trifile_1;
  my $trifile_2;
  my @boundary_1;
  my @boundary_2;
  my $disconnect_type_1;
  my $disconnect_type_2;
  my $boundaries_ne;
  my $i;

  $trifile_2 = pop;
  $trifile_1 = pop;

  $boundaries_ne = 0;

  ($disconnect_type_1) = $trifile_1 =~ /M\d+\.UB_.\.D_(T|F)\./;
  ($disconnect_type_2) = $trifile_2 =~ /M\d+\.UB_.\.D_(T|F)\./;

  if ($disconnect_type_1 ne $disconnect_type_2)
  {
    $boundaries_ne = 1;
  }
  else 
  {
    @boundary_1 = load_boundary( $trifile_1 );
    @boundary_2 = load_boundary( $trifile_2 );

    if ((scalar @boundary_1) != (scalar @boundary_2)) {
      $boundaries_ne = 1;
    }
    else {

      @boundary_1 = remove_boundary_offset(@boundary_1);
      @boundary_2 = remove_boundary_offset(@boundary_2);

      for ( $i=0; 
            ($i<(scalar @boundary_1)) && (!$boundaries_ne); 
            $i++ ) 
      {
        if ($boundary_1[$i] ne $boundary_2[$i]) {
          $boundaries_ne = 1;
        }
      }
    }
  }

  return($boundaries_ne);
}


#############################################################################
# load_boundary
#
# Read in a trifile and parses out the boundary information.  A list is 
# returned containing the interface mode followed by a sorted list of 
# variable names that define the interface.
# 
# PARAMETERS:
#   $trifile - name of trifile
#############################################################################
sub load_boundary 
{
  my $trifile;
  my @trifile;
  my @trifile_boundary;
  my @interface_method;
  my $interface_method;
  my @tokens; 
  my $nmbr_variables; 
  my @variables; 
  my $variable_name; 
  my $frame; 

  $trifile = pop;

  open TRIFILE, "<$trifile" or die "***Error: Couldn't open trifile $trifile"; 
  @trifile = <TRIFILE>;
  close TRIFILE;

  @trifile_boundary = grep /^PC_PARTITION/, @trifile;
  @interface_method = grep /^(RIGHT|LEFT)\s*$/, @trifile;
  $interface_method = "@interface_method"; 
  chomp $interface_method; 

  ((scalar @interface_method) == 1) or die 
    "Something is wrong with trifile $trifile, perhaps trifile is incomplete\n";

  @tokens = split /\s+/, "@trifile_boundary";

  (((scalar @trifile_boundary)==1) && ((shift @tokens) eq 'PC_PARTITION')) or 
    die "Something is wrong with trifile $trifile, perhaps a variable name PC_PARTITION\n";

  $nmbr_variables = shift @tokens;

  ($nmbr_variables > 0) or die "Something is wrong with trifile $trifile\n";

  while (@tokens) 
  {
    $variable_name = shift @tokens; 
    $frame = shift @tokens;

    ((defined $variable_name) && (defined $frame)) or die 
      "Something is wrong with trifile $trifile";
 
    push @variables, "${variable_name}__${frame}";     
    push @variables, "${variable_name}__${frame}";     
  }

  @variables = sort @variables;

  return($interface_method, @variables);
}


#############################################################################
# remove_boundary_offset 
#
# This removes the frame offset form a boundary.  For example,
#   word 5 wordPosition 6 
#
# would be transformed to 
#   word 0 wordPosition 1 
# 
# PARAMETERS:
#   $trifile - name of trifile
#############################################################################
sub remove_boundary_offset 
{
  my @boundary = @_;
  my @new_boundary; 
  my $minimum_frame = 9e9;
  my $variable;
  my $name;
  my $frame;

  push @new_boundary, shift @boundary;

  foreach $variable (@boundary)
  {
    ($frame) = $variable =~ /__(\d+)$/; 
    if ($frame < $minimum_frame) {
      $minimum_frame = $frame;
    }
  }

  foreach $variable (@boundary)
  {
    ($name, $frame) = $variable =~ /(.+)__(\d+)$/; 
    $frame -= $minimum_frame;
    push @new_boundary, "${name}__${frame}";  
  }

  return(@new_boundary);
}


################################################################################
# FUNCTIONS FOR CREATING A QUEUE OF TRIANGULATION HEURISTICS 
################################################################################

#############################################################################
# reset_basic_methods();
#
# Reset the counters that keep track of the current basic method 
#############################################################################
sub reset_basic_methods 
{
  lock($basic_method_queue_in_use);

  $basic_once_index = 0;
  $basic_iteration_index = 0;
  $basic_prefix_index    = 0;
  $basic_method_index    = 0;
  $basic_suffix_index    = 0;
  $basic_iterations_done = 0;
}

#############################################################################
# get_basic_triangulation_method( method );
#
# Get triangulation method from the basic method queue.  Returns 'DONE' after
# all methods have been returned.  A call to reset_basic_methods will start
# the queue from the beginning.
#############################################################################
sub get_basic_triangulation_method  
{
  my $method;
  my $method_line;
  my $method_name;

  lock($basic_method_queue_in_use);
  
  if ($basic_iterations_done) { 
    $method_line = 'DONE';
    $method_name = 'DONE';
  }
  elsif ($basic_once_index < (scalar @run_once)) {
    $method = $run_once[$basic_once_index];
    $method_line = "-triangulationHeuristic $method"; 
    $method_name = "$method.F"; 

    $basic_once_index++;
  }
  else 
  {
    $method = "$basic_iterations[$basic_iteration_index]$basic_prefix[$basic_prefix_index]$basic_method[$basic_method_index]";

    if ($basic_method_index >= $basic_suffix_threshold) {
      $method = "$method$basic_suffix[$basic_suffix_index]";
    }

    $method_line = "-triangulationHeuristic $method"; 
    $method_name = "$method.F"; 

    #########################################################################
    # Increment the method indices
    #########################################################################
    if ($basic_method_index >= $basic_suffix_threshold) {
      $basic_suffix_index++;
      if ($basic_suffix_index >= (scalar @basic_suffix)) {
        $basic_suffix_index = 0;
        $basic_method_index++;
      }
    }

    if ($basic_method_index < $basic_suffix_threshold) {
      $basic_method_index++;
    }

    if ($basic_method_index >= (scalar @basic_method)) {
      $basic_method_index = 0;
      $basic_prefix_index++;
      if ($basic_prefix_index >= (scalar @basic_prefix)) {
        $basic_prefix_index = 0;
        $basic_iteration_index++;
        if ($basic_iteration_index >= (scalar @basic_iterations)) {
          $basic_iteration_index = 0;
          $basic_iterations_done = 1; 
        }
      }
    } 
  }

  return($method_line, $method_name);
}

#############################################################################
# reset_advanced_methods();
#
# Reset the counters that keep track of the current advanced method 
#############################################################################
sub reset_advanced_methods 
{
  lock($advanced_method_queue_in_use);

  $advanced_once_index      = 0; 
  $advanced_UB_type_index   = 0;
  $advanced_iteration_index = 0;
  $advanced_prefix_index    = 0;
  $advanced_method_index    = 0;
  $advanced_suffix_index    = 0;
  $advanced_iterations_done = 0;
}

#############################################################################
# get_triangulation_method( method );
#
# Get triangulation method from the advanced method queue.  Returns 'DONE'
# after all methods have been returned.  A call to reset_basic_methods will
# start the queue from the beginning.
#############################################################################
sub get_advanced_triangulation_method  
{
  my $method;
  my $method_line;
  my $method_name;
  my $other_method_options;

  lock($advanced_method_queue_in_use);

  if ($advanced_iterations_done) 
  { 
    $method_line = 'DONE';
    $method_name = 'DONE';
  }
  else 
  {
    if ($advanced_once_index < (scalar @run_once)) 
    {
      $method = $run_once[$advanced_once_index];

      $method_line = "-triangulationHeuristic $method -jtwUB $advanced_UB_types[$advanced_UB_type_index]";
      $method_name = "${method}.$advanced_UB_types[$advanced_UB_type_index]";

      $advanced_UB_type_index++;
      if ($advanced_UB_type_index >= (scalar @advanced_UB_types)) 
      {
        $advanced_UB_type_index=0;
        $advanced_once_index++;
      }
    }
    else 
    {
      $method = "$advanced_iterations[$advanced_iteration_index]$advanced_prefix[$advanced_prefix_index]$advanced_method[$advanced_method_index]"; 

      if ($advanced_method_index >= $advanced_suffix_threshold) 
      {
        $method = "$method$advanced_suffix[$advanced_suffix_index]";
      }

      $method_line = "-triangulationHeuristic $method -jtwUB $advanced_UB_types[$advanced_UB_type_index]";
      $method_name = "${method}.$advanced_UB_types[$advanced_UB_type_index]";

      #################################################################
      # Increment the method indices
      #################################################################
      $advanced_UB_type_index++;
      if ($advanced_UB_type_index >= (scalar @advanced_UB_types)) 
      {
        $advanced_UB_type_index=0;

        if ($advanced_method_index >= $advanced_suffix_threshold) 
        {
          $advanced_suffix_index++;
          if ($advanced_suffix_index >= (scalar @advanced_suffix)) 
          {
            $advanced_suffix_index = 0;
            $advanced_method_index++;
          }
        }

        if ($advanced_method_index < $advanced_suffix_threshold) 
        {
          $advanced_method_index++;
        }

        if ($advanced_method_index >= (scalar @advanced_method)) 
        {
          $advanced_method_index = 0;
          $advanced_prefix_index++;
          if ($advanced_prefix_index >= (scalar @advanced_prefix)) 
          {
            $advanced_prefix_index = 0;
            $advanced_iteration_index++;
            if ($advanced_iteration_index >= (scalar @advanced_iterations)) 
            {
              $advanced_iteration_index = 0;
              $advanced_iterations_done = 1; 
            }
          }
        }
      } 
    }
  } 

  return($method_line, $method_name);
}

################################################################################
# MISC
################################################################################

#############################################################################
# get_seconds(timestring)
#
# The input is a time string specifying:  seconds, minutes, hours, days, 
# weeks.  Examples: 
#    '3 seconds 4 minutes 1 hour'  
#    '4min 1 hour 2 days'  
#    '1w3s1h'  
#
# PARAMETERS:
#  $time_string - Time in string format 
#
# Output is the integer number of seconds represented by the string.
#############################################################################
sub get_seconds 
{
  my $time_string;
  my @strings;
  my $total;
  my $string;
  my $numbers;
  my $letters;
  my $scnds;
  
  $time_string = pop;

  (@strings) = split /(\d+\s*\w+)/, $time_string;

  $total = 0;

  foreach $string (@strings)
  {
    ($numbers, $letters) = $string =~ /(\d+)\s*(\w+)/;

    if ((defined $letters) && (defined $numbers) &&  
        ('seconds' =~ /$letters/)) {
      $total = $total + $numbers;
    }
    elsif ((defined $letters) && (defined $numbers) &&  
           ('minutes' =~ /$letters/)) {
      $total = $total + 60*$numbers;
    }
    elsif ((defined $letters) && (defined $numbers) &&  
           ('hours' =~ /$letters/)) {
      $total = $total + 60*60*$numbers;
    }
    elsif ((defined $letters) && (defined $numbers) &&  
           ('days' =~ /$letters/)) {
      $total = $total + 24*60*60*$numbers;
    }
    elsif ((defined $letters) && (defined $numbers) &&  
           ('weeks' =~ /$letters/)) {
      $total = $total + 7*24*60*60*$numbers;
    }
  }

  return $total;
}


#############################################################################
# infoMsg 
#   Displays information based on the current verbosity level 
# 
# PARAMETERS:
#   Required verbosity level 
#   printf parameters
#############################################################################
sub infoMsg 
{
  my $required_verbosity = shift;

  if ($verbosity >= $required_verbosity)
  {
    print "INFO:";
    print time;
    print " ";
    printf(@_);
  }
}


#############################################################################
# get_gmtkTime_options 
#
# PARAMETERS:
#   none   
#############################################################################
sub get_gmtkTime_options 
{
  my $gmtkTime_options = pop;
  my $get_options_hash = pop;
  my $arg;
  my $time_arg;
  my $line;
  my $array_test;
  my @args_code;
  my @args_file;
  my @args;

  my $gmtkTime_location = $FindBin::Bin;

  if (open GMTKTIME, "<$gmtk_args_h")
  {
    #####################################################################
    # Get the arguments from gmtkTime.cc
    #####################################################################
    @args_code = grep /Arg.*\(.*\).*,/, <GMTKTIME>;

    foreach $line (@args_code)
    {
      ($arg) = $line =~ /Arg\(\s*\"([^\"]+)\"/;    
      ($array_test) = $line =~ /Arg::ARRAY/;

      if (defined $arg) {
        push @args, my $new_arg = { NAME=>$arg, ARRAY=>$array_test };
      }
    }

    #####################################################################
    # Add the arguments to triangulateTimings' arguments 
    #####################################################################
    foreach $time_arg (@args)
    {
      if ( ($time_arg->{NAME} ne 'seconds') &&
           ($time_arg->{NAME} ne 'strFile') && 
           ($time_arg->{NAME} ne 'verbosity') ) 
      {
        if (defined $time_arg->{ARRAY})
        {
          #####################################################################
          # If an array argument (such as -of1, -of2) add integers to name 
          #####################################################################
          for (my $i=1; $i<10; $i++)
          {
            $get_options_hash->{"$time_arg->{NAME}$i:s"} = 
              \$gmtkTime_options->{"$time_arg->{NAME}$i"};
          }
        }
        else 
        {
          $get_options_hash->{"$time_arg->{NAME}:s"} = 
            \$gmtkTime_options->{"$time_arg->{NAME}"};
        }
      }
    } 
  }
  else
  { 
    print "${warning_message}Can't open gmtkTime.cc, gmtkTime options are unavailable: was trying $gmtk_args_h\n";
  }

}


#############################################################################
# make_gmtkTime_script 
#
# PARAMETERS:
#
#############################################################################
sub make_gmtkTime_script 
{
  my $gmtkTime_options = pop;
  my $option;
  my $script_name;

  $script_name = "./$output_directory/ulimit_job"; 

  open TIMESCRIPT, ">$script_name" or die "${error_message}Couldn't create file $output_directory/ulimit_job\n";

  print TIMESCRIPT "#!/bin/bash\n";
  print TIMESCRIPT "set -x\n";
  print TIMESCRIPT "ulimit -SHv 1000000\n";
  print TIMESCRIPT "$gmtk_time -probE -strFile $str_file ";  
  foreach $option (keys %{$gmtkTime_options}) 
  {
    if (defined $gmtkTime_options->{$option}) 
    {
      print TIMESCRIPT "-$option $gmtkTime_options->{$option} ";
    }
  }
  print TIMESCRIPT " \$\*\n";

  close TIMESCRIPT; 
  `chmod u+x $script_name`;

  return($script_name);
}

#############################################################################
# check_timing_and_export_parameters
#
# PARAMETERS:
#
#############################################################################
sub check_timing_and_export_parameters
{
  my $command;
  my $result;
  my @result;

  print "Checking ability to triangulate...\n"; 
  $command = "$gmtk_triangulate -strFile $str_file -rePartition T -reTriangulate T -findBestBoundary F -outputTriangulatedFile $output_directory/initial.trifile -numBackupFiles 0 -triangulationH completed -disconnectFromObservedParent T 2>&1";
  print STDERR "$command\n";
  $result = `$command`;
  print STDERR "$result";
  if ($result =~ /____ PROGRAM ENDED SUCCESSFULLY WITH STATUS 0/) {
    print "  Triangulation appears to have run successfully\n";
    print "  (result in $output_directory/initial.trifile)\n";
  }
  else {
    print "${error_message}Problem running gmtkTriangulate\n";  
    print "   Might be a problem in the structure file, or gmtkTriangulate\n"; 
    print "   is not in your path\n";  
    die "${error_message}Problem running gmtkTriangulate\n";  
  }

  print "Checking timing script...\n"; 
  $result = `$timing_script -triFile $output_directory/initial.trifile -seconds 1 -ckbeam 1 2>&1`; 
  print STDERR "$result\n";
  if ($result =~ /____ PROGRAM ENDED SUCCESSFULLY WITH STATUS 0/) {
    print "  timing script appears to have run successfully\n";
  }
  else {
    print "${error_message}Problem running timing script\n";  
    die "${error_message}Problem running timing script\n";  
  }

  print "Checking boundaryExportLine...\n"; 
  $command = "$boundary_export_line echo testing_boundaryExportLine";
  print STDERR "$command\n";
  $result = `$command`;
  print STDERR "$result";  
  @result = split /\n/, $result;
  if (grep /^testing_boundaryExportLine$/, @result)
  {
    print "  export appears to have run succesfully\n"; 
  }
  else {
    print "${error_message}Problem running boundaryExportLine, must be able to pipe commands to stdin\n";  
    die "${error_message}Problem running boundaryExportLine, must be able to pipe commands to stdin\n";  
  }

  print "Checking triangulationExportLine...\n"; 
  $command = "$triangulation_export_line echo testing_triangulationExportLine"; 
  print STDERR "$command\n";
  $result = `$command`;
  print STDERR "$result";  
  @result = split /\n/, $result;
  if (grep /^testing_triangulationExportLine$/, @result)
  {
    print "  export appears to have run succesfully\n"; 
  }
  else {
    print "${error_message}Problem running triangulationExportLine, must be able to accept commands as parameters \n";  
    die "${error_message}Problem running triangulationExportLine, must be able to accept commands as parameters \n";  
  }

  print "Checking timingExportLine...\n"; 
  $command = "$timing_export_line echo testing_timingExportLine"; 
  print STDERR "$command\n";
  $result = `$command`;
  print STDERR "$result";  
  @result = split /\n/, $result;
  if (grep /^testing_timingExportLine$/, @result)
  {
    print "  export appears to have run succesfully\n"; 
  }
  else {
    print "${warning_message}Problem running timingExportLine, must be able to accept commands as parameters, continuing because the issue might be caused from no hosts being available\n";  
  }

}



#############################################################################
# POD 
#############################################################################

=head1 NAME
  
triangulateTimings - generate and time GMTK triangulations

=head1 SYNOPSIS

triangulateTimings -strFile file -timingExportLine script [OPTIONS] [input triangulations] 

The options are followed by an optional list of trifiles.  These input
triangulations are timed and their boundaries are used in further
triangulations.  Typically these are the best triangulations from previous
searches and are given as starting points for improvements and as points of
comparison. 

The output from the GMTK processes is sent to standard error.  For a concise
version of the script's progress only view standard out. 

Example:

triangulateTimings -strFile hmm.str -timingExportLine "gmtkTime -of1 data -inputMaster masterfile" -outputDirectory triangulation_dir old_triangulation_A.trifile old_triangulation_B.trifile 

=head1 OPTIONS

=over

=item <-strFile I<file>>

GMTK structure file 

=item <-timingScript I<script>> 

Script which runs gmtkTime for the given structure file.  This can be a command
line in quotes which calls gmtkTime, or a script file.  If using a script file,
the script must add any command line options given to it to its call to
gmtkTime (typically using $*).

=item [-short]

Default settings for undefined parameters are for a short triangulation run.

=item [-medium]

Default settings for undefined parameters are for a medium length triangulation
run.

=item [-long]

Default settings for undefined parameters are for a long triangulation run.

=item [-boundaryExportLine I<script>] 

This should specify a command which reads a list of commands from its standard
input and exports these processes to remote computers.     

=item [-triangulationExportLine I<script>] 

Commands which proceed a call to gmtkTriangulate to export the process to a
remote computer.   

=item [-timingExportLine I<script>] 

Commands which proceed a call to the timing script which should be used to
export the process to a remote computer.   

=item [-parallelism I<integer>] 

This controls the parallelism of the timing and triangulation processes.  This
is independent of the number of processes in the boundary search.  If the
parallelism is set to 1, measures are taken to make sure nothing is run at the
same time as the timing script.  If parallelism is greater than 1, the
timingExportLine line should be used to make sure gmtkTime has exclusive use of
a processor.   

=item [-outputDirectory I<directory>] 

Directory to store trifiles. 

=item [-throttling I<integer>] 

The number of seconds to wait between submitting jobs with the export lines (e.g. to GridEngine)

=item [-seconds I<integer>] 

The number of seconds to time the triangulation.  

=item [-boundaryTime I<string>] 

String giving the amount of time spent on each boundary search.  The string can
specify seconds, minutes, hours, days, or weeks.  Examples: '3 seconds 4
minutes 1 hour',  '4min 1 hour 2 days',  '1w3s1h',  

=item [-maximumBoundarySearches I<integer>] 

The maximum number of boundary searches to be run.  The maximum and choice of
boundary searches is allow affected by the -short, -medium, and -long switches.

=item [-useExistingBoundaries] 

When this flag is set no boundary searches will be run.  The boundaries will be
taken from the the input triangulations and the .boundary files in the output
directory and.  

=item [-triangulateP] 

This determines if a search for a prologue triangulation are performed.  Can be
turned off with -noTriangulateP. 

=item [-triangulateC] 

This determines if a search for a chunk triangulation is performed.  Can be
turned off with -noTriangulateC. 

=item [-triangulateE] 

This determines if a search for a epilogue triangulation is performed.  Can be
turned off with -noTriangulateE. 

=back

=cut

