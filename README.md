
<!-- README.md is generated from README.Rmd. Please edit that file -->

# NAIR: Network Analysis For Immune Repertoire

<!-- badges: start -->
<!-- badges: end -->

The `NAIR` package facilitates network analysis of the adaptive immune
repertoire based on similarities among the receptor sequences. It
implements custom pipelines developed in the following paper:

Hai Yang, Jason Cham, Zenghua Fan, Brian Neal, Tao He and Li Zhang.
“Network Analysis of Immune Repertoire (NAIR) with Advanced Machine
Learning Techniques.” In: Briefings in Bioinformatics (under review).

### What can `NAIR` do?

`NAIR` allows the user to:

- Perform general network analysis on immune repertoire sequence
  (RepSeq) data, including computing local and global network properties
  of nodes and clusters
- Search across multiple RepSeq samples for:
- Clones/clusters associated to a clinical outcome
- Public clones/clusters
- Generate customized visualizations of the immune repertoire network
- Perform further downstream analysis

### What data does `NAIR` support?

`NAIR` supports bulk and single-cell immune repertoire sequence data for
T-cell or B-cell receptors (TCR or BCR).

- **Single-cell data:** Each row is a single T-cell/B-cell
- **Bulk data:** Each row is a distinct TCR/BCR clone (unique
  combination of V-D-J genes and nucleotide sequence) and typically
  includes a corresponding measurement of clonal abundance (e.g., clone
  count and clone frequency/fraction)

### How does `NAIR` model the immune repertoire as a network?

- Each TCR/BCR cell (single-cell data) or clone (bulk data) is modeled
  as a node (vertex) in the network
- For each node, we consider the corresponding receptor sequence
  (nucleotide or amino acid)
- For each pair of nodes, we measure the similarity in their receptor
  sequences (using the Hamming or Levenshtein distance)
- An edge is drawn between two nodes if the distance is below a
  specified threshold

## Installation

You can install the development version of `RepSeqNetworkAnalysis` from
GitHub using the following commands in `R`:

    install.packages("devtools")
    devtools::install_github("mlizhangx/Network-Analysis-for-Repertoire-Sequencing-",
                             build_vignettes = TRUE)

Installing the development version requires having a compiling toolchain
installed (e.g., Rtools if on Windows; [Xcode Command Line Tools and
gfortran](https://thecoatlessprofessor.com/programming/cpp/r-compiler-tools-for-rcpp-on-macos/)
if on macOS).

If you run into installation issues, consider updating `R` if using a
version prior to 3.0.2. Package installation requires
sufficiently-recent versions of the `R` packages `Rcpp` (1.0.8 or later)
and `RcppArmadillo` (10.8 or later); these version requirements are
automatically imposed via the `Imports` field of the `DESCRIPTION` file;
however, including minimum version requirements in the `Imports` field
is only supported in `R` from 3.0.2 onward.

**A note to Linux users:** If installing on Linux, note that OpenMP
support is disabled by default as MacOS does not support it. If you wish
to enable OpenMP, you can do so by editing the file src/Makevars;
editing instructions are included as comments in the file. OpenMP is
used by the Armadillo library for C++ to automatically parallelize
expensive operations such as elementwise matrix operations. Having
OpenMP enabled may shorten computation time for the network adjacency
matrix depending on the extent to which Armadillo’s methods for sparse
matrices take advantage of it.

### Python dependency

Certain niche functions within this package require Python version 3.6
or greater. The dependency is handled automatically in a lazy-loading
fashion thanks to the `reticulate` package; if a Python installation is
not found when a dependent function is called, the user will be
automatically prompted to install the latest miniconda Python
distribution, and a Python environment will be automatically set up.

`reticulate` will attempt to install any Python modules required by
functions in our package when setting up the Python environment. Package
functions that require Python modules to be available check first for
their availability. If they are unavailable, our package includes a
function `installPythonModules()` that can be called to check for and
dispatch calls to `reticulate::py_install()` in order to install any
missing modules.

If you wish to specify the Python environment to use, you can do so by
using the `use_virtualenv()` function from the `reticulate` package
prior to calling functions from `RepSeqNetworkAnalysis`.

### Vignettes and Help Files

Once the package is installed, you can view the help directory in the
`R` help pane by calling `help(package = "NAIR")`. This contains a
directory of documentation files for individual package functions, as
well as an introductory vignette accessible via the item
`User guides, package vignettes and other documentation`. This vignette,
whose content is reproduced below, can also be viewed in your web
browser by calling `browseVignettes("NAIR")`.

# The `buildRepSeqNetwork()` function

General network analysis on RepSeq data is performed using the
`buildRepSeqNetwork()` function. This function does the following:

- Builds the network graph for the immune repertoire
- Computes desired network properties
- Prints a customized `ggraph` plot of the network graph
- Returns meta-data for the TCR/BCR (nodes) in the network, including
  biological as well as network properties
- If desired for downstream analysis, can also return the network
  `igraph` and adjacency matrix, as well as the `ggraph` plot object

### Load Data

For demonstration purposes, we load a [public data
set](https://www.10xgenomics.com/resources/datasets/pbm-cs-of-a-healthy-donor-v-1-1-1-standard-3-1-0)
hosted at 10xgenomics.com using Bioconductor.

``` r
if (!require("BiocFileCache", quietly = TRUE)) { 
  if (!require("BiocManager", quietly = TRUE)) {
    install.packages("BiocManager")
  }
  BiocManager::install("BiocFileCache", update = FALSE) 
}
library(NAIR)
library(BiocFileCache)
bfc <- BiocFileCache(ask = FALSE)
data_file <- bfcrpath(bfc, file.path(
  "http://cf.10xgenomics.com/samples/cell-vdj/3.1.0",
  "vdj_v1_hs_pbmc3/vdj_v1_hs_pbmc3_t_filtered_contig_annotations.csv"))
tcr_data <- read.csv(data_file, stringsAsFactors = FALSE)
head(tcr_data)
#>              barcode is_cell                   contig_id high_confidence length
#> 1 AAACCTGAGATCTGAA-1    True AAACCTGAGATCTGAA-1_contig_1            True    521
#> 2 AAACCTGAGATCTGAA-1    True AAACCTGAGATCTGAA-1_contig_2            True    474
#> 3 AAACCTGAGGAACTGC-1    True AAACCTGAGGAACTGC-1_contig_1            True    496
#> 4 AAACCTGAGGAACTGC-1    True AAACCTGAGGAACTGC-1_contig_2            True    505
#> 5 AAACCTGAGGAGTCTG-1    True AAACCTGAGGAGTCTG-1_contig_1            True    495
#> 6 AAACCTGAGGAGTCTG-1    True AAACCTGAGGAGTCTG-1_contig_2            True    526
#>   chain     v_gene d_gene  j_gene c_gene full_length productive
#> 1   TRB   TRBV20-1   None TRBJ2-7  TRBC2        True       True
#> 2   TRA   TRAV13-1   None  TRAJ44   TRAC        True       True
#> 3   TRB    TRBV7-2   None TRBJ2-1  TRBC2        True       True
#> 4   TRA TRAV23/DV6   None  TRAJ34   TRAC        True       True
#> 5   TRA      TRAV2   None  TRAJ38   TRAC        True       True
#> 6   TRB    TRBV6-2   None TRBJ1-1  TRBC1        True       True
#>                 cdr3                                                cdr3_nt
#> 1     CSARDKGLSYEQYF             TGCAGTGCTAGAGACAAGGGGCTTAGCTACGAGCAGTACTTC
#> 2 CAASIGPLGTGTASKLTF TGTGCAGCAAGTATCGGCCCCCTAGGAACCGGCACTGCCAGTAAACTCACCTTT
#> 3      CASSLGPSGEQFF                TGTGCCAGCAGCTTGGGACCATCGGGTGAGCAGTTCTTC
#> 4       CAASDNTDKLIF                   TGTGCAGCAAGCGATAACACCGACAAGCTCATCTTT
#> 5   CAVEANNAGNNRKLIW       TGTGCTGTGGAGGCTAATAATGCTGGCAACAACCGTAAGCTGATTTGG
#> 6      CASSRTGGTEAFF                TGTGCCAGCAGTCGGACAGGGGGCACTGAAGCTTTCTTT
#>   reads umis raw_clonotype_id         raw_consensus_id
#> 1  9327   12     clonotype100 clonotype100_consensus_1
#> 2  3440    3     clonotype100 clonotype100_consensus_2
#> 3 32991   29     clonotype101 clonotype101_consensus_2
#> 4 10714    9     clonotype101 clonotype101_consensus_1
#> 5  1734    3     clonotype102 clonotype102_consensus_1
#> 6 15530   13     clonotype102 clonotype102_consensus_2

# load data sets
# dir_main <-
#   dirname(dirname(rstudioapi::getActiveDocumentContext()$path))
# dir_data <- file.path(dir_main, "input_data")
# dir_out <- file.path(dir_data, "dummy_output")
# 
# data <- read.table(
#   file.path(dir_data, "TRB-Pt-10-2-500ng-15-04-2020-gDNA_S48.clones.txt"),
#   sep = '\t', header = TRUE)
```

Each row corresponds to a TCR sequence from a single cell, with the cell
ID contained in the first column, `barcode`. Sequences from both the
beta and alpha chains are present, as denoted by the values `TRA` and
`TRB` in the column `chain`. We consider the beta-chain CDR3 amino acid
sequences.

``` r
# Subset rows for valid beta-chain CDR3 amino acid sequences
tcr_data <- tcr_data[tcr_data$chain == "TRB" & tcr_data$cdr3 != "None", ]

# Number of sequences/cells (rows)
nrow(tcr_data)
#> [1] 4206
```

## Basic Usage

Calling `buildRepSeqNetwork()` for our example is as simple as executing
the following line of code:

``` r
buildRepSeqNetwork(tcr_data, "cdr3")
```

- The first argument specifies the data frame containing the rep-seq
  data, where each row corresponds to a single TCR/BCR clone (bulk data)
  or cell (single-cell data).
- The second argument specifies the column name or number of the data
  frame that contains the receptor sequences to be used as the basis of
  similarity between two cells or clones.

What does it do by default? Let’s observe the side effects and output
when we call the function using the default settings:

``` r
output <- buildRepSeqNetwork(tcr_data, "cdr3")
#> Input data contains 4206 rows.
#> Removing sequences with length less than 3... Done. 4206 rows remaining.
#> Computing network edges based on a max hamming distance of 1... Done.
#> 588 nodes in the network (after removing nodes with degree zero).
#> Generating graph plot...
```

<img src="man/figures/README-unnamed-chunk-5-1.png" width="100%" style="display: block; margin: auto;" />

    #>  Done.
    #> Finished building network.

The console messages indicate the following tasks being performed:

- The input data is filtered to remove rows with sequences below 3
  characters in length
- The edges of the network graph are computed by calculating the Hamming
  distance between pairs of the receptor sequences in `tcr_data$cdr3`
- Isolated nodes (those not joined by an edge to any other node) are
  removed from the graph
- A plot of the network graph is generated

By default, a data frame is returned with the same columns as the input
data:

``` r
names(output)
#>  [1] "barcode"          "is_cell"          "contig_id"        "high_confidence" 
#>  [5] "length"           "chain"            "v_gene"           "d_gene"          
#>  [9] "j_gene"           "c_gene"           "full_length"      "productive"      
#> [13] "cdr3"             "cdr3_nt"          "reads"            "umis"            
#> [17] "raw_clonotype_id" "raw_consensus_id"
```

Only rows corresponding to nodes that remain in the network graph are
included (those corresponding to the dropped isolated nodes have been
removed):

``` r
nrow(output)
#> [1] 588
```

Thus, this output data serves as biological meta-data for the nodes in
the network graph, with each row corresponding to a node seen in the
plot above.

## Network Properties

To include network properties in the output of `buildRepSeqNetwork()`,
we use the `node_stats` and `cluster_stats` arguments; these
respectively specify whether node-level and cluster-level properties are
computed.

### Node-Level Network Properties

Use `node_stats = TRUE` to include node-level network properties.

``` r
# Node-level properties
output <- buildRepSeqNetwork(tcr_data, "cdr3", node_stats = TRUE)
#> Input data contains 4206 rows.
#> Removing sequences with length less than 3... Done. 4206 rows remaining.
#> Computing network edges based on a max hamming distance of 1... Done.
#> 588 nodes in the network (after removing nodes with degree zero).
#> Computing node-level network statistics... Done.
#> Generating graph plot with nodes colored by degree...
```

<img src="man/figures/README-unnamed-chunk-8-1.png" width="100%" style="display: block; margin: auto;" />

    #>  Done.
    #> Finished building network.

The output data now contains node-level network properties in addition
to the biological meta-data:

``` r
names(output)
#>  [1] "barcode"                   "is_cell"                  
#>  [3] "contig_id"                 "high_confidence"          
#>  [5] "length"                    "chain"                    
#>  [7] "v_gene"                    "d_gene"                   
#>  [9] "j_gene"                    "c_gene"                   
#> [11] "full_length"               "productive"               
#> [13] "cdr3"                      "cdr3_nt"                  
#> [15] "reads"                     "umis"                     
#> [17] "raw_clonotype_id"          "raw_consensus_id"         
#> [19] "degree"                    "transitivity"             
#> [21] "eigen_centrality"          "centrality_by_eigen"      
#> [23] "betweenness"               "centrality_by_betweenness"
#> [25] "authority_score"           "coreness"                 
#> [27] "page_rank"
```

We also notice in the plot that nodes are now colored by network degree,
which is one of the included node-level network properties.

#### Choosing Node-Level Properties

To choose which node-level network properties are computed, use the
`node_stat_settings()` function, passing its output to the
`stats_to_include` argument of `buildRepSeqNetwork()`. The name of each
node-level network property has a matching argument in
`node_stat_settings()` that can be set to `TRUE` or `FALSE` to specify
its inclusion. By default, calling `buildRepSeqNetwork()` with
`node_stats = TRUE` uses the default values of `node_stat_settings()`,
which are as follows:

``` r
node_stat_settings()
#> $degree
#> [1] TRUE
#> 
#> $cluster_id
#> [1] FALSE
#> 
#> $transitivity
#> [1] TRUE
#> 
#> $closeness
#> [1] FALSE
#> 
#> $centrality_by_closeness
#> [1] FALSE
#> 
#> $eigen_centrality
#> [1] TRUE
#> 
#> $centrality_by_eigen
#> [1] TRUE
#> 
#> $betweenness
#> [1] TRUE
#> 
#> $centrality_by_betweenness
#> [1] TRUE
#> 
#> $authority_score
#> [1] TRUE
#> 
#> $coreness
#> [1] TRUE
#> 
#> $page_rank
#> [1] TRUE
#> 
#> $all_stats
#> [1] FALSE
```

See `?node_stat_settings()` for more details on the individual
properties.

#### Include All Node-Level Properties

A convenient way to include all node-level network stats when calling
`node_stat_settings()` is to simply use the argument `all_stats = TRUE`,
which overrides the values of its other arguments.

When calling `buildRepSeqNetwork()` with `node_stats = TRUE`, a further
shortcut to include all node-level network stats is simply to use
`stats_to_include = "all"`.

### Cluster-Level Network Properties

We can include cluster-level network properties using
`cluster_stats = TRUE`:

``` r
# Node-level and cluster-level properties
output <- buildRepSeqNetwork(tcr_data, "cdr3", node_stats = TRUE, 
                             cluster_stats = TRUE, print_plots = FALSE)
#> Input data contains 4206 rows.
#> Removing sequences with length less than 3... Done. 4206 rows remaining.
#> Computing network edges based on a max hamming distance of 1... Done.
#> 588 nodes in the network (after removing nodes with degree zero).
#> Computing node-level network statistics... Done.
#> Computing cluster membership within the network... Done.
#> Computing statistics for the 212 clusters in the network... Done.
#> Generating graph plot with nodes colored by degree... Done.
#> Finished building network.
```

The output is no longer a single data frame; instead, it is now a list
containing two data frames, one for the node-level meta data and one for
the cluster-level meta data:

``` r
names(output)
#> [1] "node_data"     "cluster_stats"
head(output$cluster_stats)
#>   cluster_id node_count mean_seq_length mean_degree max_degree
#> 1          1         32              16          31         31
#> 2          2         12              19          11         11
#> 3          3         11              16          10         10
#> 4          4          8              17           7          7
#> 5          5          7              14           6          6
#> 6          6          6              15           5          5
#>      seq_w_max_degree agg_clone_count max_clone_count seq_w_max_count
#> 1    CASSLEVGGGEETQYF              NA              NA              NA
#> 2 CASSEAPETSGTKGYEQFF              NA              NA              NA
#> 3    CASSQLLGQGPYEQYF              NA              NA              NA
#> 4   CASSQEASQEPYNEQFF              NA              NA              NA
#> 5      CASMGATASYEQYF              NA              NA              NA
#> 6     CASSQDSSSNSPLHF              NA              NA              NA
#>   diameter_length assortativity transitivity edge_density
#> 1               2           NaN            1            1
#> 2               2           NaN            1            1
#> 3               2           NaN            1            1
#> 4               2           NaN            1            1
#> 5               2           NaN            1            1
#> 6               2           NaN            1            1
#>   degree_centrality_index closeness_centrality_index eigen_centrality_index
#> 1                       0                          0           1.184238e-16
#> 2                       0                          0           1.776357e-16
#> 3                       0                          0           0.000000e+00
#> 4                       0                          0           2.960595e-16
#> 5                       0                          0           1.776357e-16
#> 6                       0                          0           2.220446e-16
#>   eigen_centrality_eigenvalue
#> 1                          31
#> 2                          11
#> 3                          10
#> 4                           7
#> 5                           6
#> 6                           5
```

Each row of the cluster-level meta data corresponds to a single cluster
in the network.

Three of the cluster-level properties, `agg_clone_count`,
`max_clone_count` and `seq_w_max_count`, were not computed, because they
require us to specify the column name or number of our data containing
measurements of clonal/cell abundance via the `count_col` argument. If
the column name or number is supplied, these properties will be
automatically computed along with the other cluster-level properties.

## Customized Visualization

The network graph plot produced by `buildRepSeqNetwork()` can be
customized in various ways.

### Color nodes using meta data

The nodes in the graph can be colored according to node-level meta-data
by specifying a variable name in the `color_nodes_by` argument. This can
be a variable from the input data or one of the node-level network
properties included in the output.

For example, we can color the nodes based on the `transitivity` network
property:

``` r
output <- buildRepSeqNetwork(tcr_data, "cdr3", node_stats = TRUE,
                             color_nodes_by = "transitivity")
#> Input data contains 4206 rows.
#> Removing sequences with length less than 3... Done. 4206 rows remaining.
#> Computing network edges based on a max hamming distance of 1... Done.
#> 588 nodes in the network (after removing nodes with degree zero).
#> Computing node-level network statistics... Done.
#> Generating graph plot with nodes colored by transitivity...
```

<img src="man/figures/README-unnamed-chunk-13-1.png" width="100%" style="display: block; margin: auto;" />

    #>  Done.
    #> Finished building network.

### Adjust node color palette

The color palette used to color the nodes can be specified using the
`color_scheme` argument. Options include:

- `"default"` for default `ggplot2` colors
- A viridis color map option (e.g., `"viridis"`, `"plasma"`, etc., or
  one the corresponding strings “A” through “H”; see `?viridis` for
  details). The direction of the color gradient can be reversed by
  appending `"-1"`, as in `"viridis-1"`.
- A palette from `grDevices::hcl.pals()` (these can only be used if the
  variable used to color the nodes is discrete).

``` r
# Using the "plasma" color scheme with reversed color gradient
output <- buildRepSeqNetwork(tcr_data, "cdr3",
                             node_stats = TRUE,
                             color_nodes_by = "transitivity",
                             color_scheme = "plasma-1")
#> Input data contains 4206 rows.
#> Removing sequences with length less than 3... Done. 4206 rows remaining.
#> Computing network edges based on a max hamming distance of 1... Done.
#> 588 nodes in the network (after removing nodes with degree zero).
#> Computing node-level network statistics... Done.
#> Generating graph plot with nodes colored by transitivity...
```

<img src="man/figures/README-unnamed-chunk-14-1.png" width="100%" style="display: block; margin: auto;" />

    #>  Done.
    #> Finished building network.

### Adjust node size

The default fixed node size is quite small, which is intended to prevent
nodes from overlapping and obscuring edges in larger networks. The node
size can be adjusted by supplying a numeric value to the argument
`size_nodes_by` (the default value is 0.5):

``` r
output <- buildRepSeqNetwork(tcr_data, "cdr3",
                             node_stats = TRUE,
                             color_nodes_by = "transitivity",
                             color_scheme = "plasma-1",
                             size_nodes_by = 1)
#> Input data contains 4206 rows.
#> Removing sequences with length less than 3... Done. 4206 rows remaining.
#> Computing network edges based on a max hamming distance of 1... Done.
#> 588 nodes in the network (after removing nodes with degree zero).
#> Computing node-level network statistics... Done.
#> Generating graph plot with nodes colored by transitivity...
```

<img src="man/figures/README-unnamed-chunk-15-1.png" width="100%" style="display: block; margin: auto;" />

    #>  Done.
    #> Finished building network.

A value of `NULL` will cause the default `ggraph` node sizes to be used.

### Size nodes using meta data

The `size_nodes_by` argument also allows nodes to be sized dynamically
based on node-level meta data by supplying a variable name, similarly to
`color_nodes_by`.

When using dynamic node sizes, the minimum and maximum node sizes can be
specified as a length-2 numeric vector to the `node_size_limits`
argument. The default value of `NULL` uses the default range of node
sizes in `ggraph`.

``` r
# Size nodes dynamically by degree; use custom size range
output <- buildRepSeqNetwork(tcr_data, "cdr3",
                             node_stats = TRUE,
                             color_nodes_by = "transitivity",
                             size_nodes_by = "degree",
                             node_size_limits = c(0.5, 1.5))
#> Input data contains 4206 rows.
#> Removing sequences with length less than 3... Done. 4206 rows remaining.
#> Computing network edges based on a max hamming distance of 1... Done.
#> 588 nodes in the network (after removing nodes with degree zero).
#> Computing node-level network statistics... Done.
#> Generating graph plot with nodes colored by transitivity...
```

<img src="man/figures/README-unnamed-chunk-16-1.png" width="100%" style="display: block; margin: auto;" />

    #>  Done.
    #> Finished building network.

### Generate multiple graphs

It is possible to generate multiple plots, each using a different
variable to color the nodes, as follows:

``` r
output <- buildRepSeqNetwork(tcr_data, "cdr3",
                             node_stats = TRUE,
                             color_nodes_by = c("transitivity", "eigen_centrality"),
                             color_scheme = c("plasma-1", "default"),
                             size_nodes_by = "degree",
                             node_size_limits = c(0.5, 1.5))
#> Input data contains 4206 rows.
#> Removing sequences with length less than 3... Done. 4206 rows remaining.
#> Computing network edges based on a max hamming distance of 1... Done.
#> 588 nodes in the network (after removing nodes with degree zero).
#> Computing node-level network statistics... Done.
#> Generating graph plot with nodes colored by transitivity...
```

<img src="man/figures/README-unnamed-chunk-17-1.png" width="100%" style="display: block; margin: auto;" />

    #>  Done.
    #> Generating graph plot with nodes colored by eigen_centrality...

<img src="man/figures/README-unnamed-chunk-17-2.png" width="100%" style="display: block; margin: auto;" />

    #>  Done.
    #> Finished building network.

If a single value is supplied for `color_scheme`, it will be used for
all of the plots.

### Plot title/subtitle

The plot title and subtitle can be specified using the `plot_title` and
`plot_subtitle` arguments, or omitted by supplying a `NULL` value:

``` r
output <- buildRepSeqNetwork(tcr_data, "cdr3",
                             plot_title = NULL,
                             plot_subtitle = NULL)
#> Input data contains 4206 rows.
#> Removing sequences with length less than 3... Done. 4206 rows remaining.
#> Computing network edges based on a max hamming distance of 1... Done.
#> 588 nodes in the network (after removing nodes with degree zero).
#> Generating graph plot...
```

<img src="man/figures/README-unnamed-chunk-18-1.png" width="100%" style="display: block; margin: auto;" />

    #>  Done.
    #> Finished building network.

### Legends and legend titles

The legends for node color and size can be toggled using the logical
arguments `color_legend` and `size_legend` (these are `TRUE` by
default), while custom legend titles can be specified using the
`color_title` and `size_title` arguments. Supplying a `NULL` value for a
legend title will omit the legend title.

### Edge width

The thickness of the graph edges can be adjusted using the `edge_width`
argument (the default value is 0.1).

### Further customization

The `ggraph` object for each plot generated by `buildRepSeqNetwork()`
can be returned for downstream use with the argument
`return_all = TRUE`. It can be manipulated using functions from the
`ggplot2` package much like any plot created with the `ggplot` function.
This provides the user total control to modify plots to their liking.

## Network Settings

Several arguments exist for customization of the settings used to build
the network:

### Keep Isolated Nodes (Single-Node Clusters)

By default, any nodes in the network graph that do not share an edge
with any other network node are removed, along with their corresponding
rows in the data. There are typically many of these single, isolated
nodes in a repertoire network; they are usually of little interest and
their inclusion tends to clutter the visualization of the network graph.

However, sometimes it is desirable to keep all nodes in the network,
including the isolated nodes. This can be done using the argument
`drop_isolated_nodes = FALSE`.

### Distance Function

By default, similarity between TCR/BCR sequences is based on the Hamming
distance, i.e., the number of non-matching characters in two sequences
of equal length. When sequence lengths do not match, we effectively
extend the shorter sequence, with the additional terms treated as
non-matching with those of the longer sequence.

Our package also supports the Levenshtein (edit) distance, which
measures the minimum number of single-character edits (insertions,
deletions and transformations) required to transform one sequence into
the other. It can be used in place of the Hamming distance with the
argument `dist_type = "levenshtein"`. Here we use the Levenshtein
distance to build a network based off of similarity in the CDR-3
nucleotide sequences.

``` r
# Network based on Levenshtein distance
output <- buildRepSeqNetwork(tcr_data, "cdr3_nt", dist_type = "levenshtein")
```

The Levenshtein distance innately applies to sequences of differing
lengths and can correctly account for insertions and deletions, but is
more computationally expensive, which could potentially pose challenges
when working with very large data sets and using very long TCR/BCR
sequences.

### Distance Cutoff

The distance function specified in the `dist_type` argument is used to
model the similarity between TCR/BCR sequences. By default, two nodes in
the network graph share an edge if their distance, as measured by this
function, is at most 1. This cutoff value of 1 can be set to a different
value, if desired, using the `dist_cutoff` argument.

## Other Arguments

### Input Filtering

- `min_seq_length` can be used to filter out TCR/BCR sequences by
  minimum length. Data rows with sequence lengths below this value will
  be dropped before computing the network graph. The default is 3.
- `drop_chars` can be used to filter out TCR/BCR sequences by content.
  It takes a character string or regular expression and checks each
  TCR/BCR sequence for a match; data rows with matches are dropped
  before computing the network graph.

### Only Keep Specific Columns of Input Data

By default, the node-level meta data returned by `buildRepSeqNetwork()`
includes all columns of the input data. If you only wish for specific
columns to be retained, specify these in a vector of column names or
column numbers to the `other_cols` argument. Any relevant columns used
by `buildRepSeqNetwork()`, such as the TCR/BCR sequence column specified
by `seq_col`, as well as any columns specified in other arguments, such
as `color_nodes_by`, will automatically be included.

### Output and Side Effects

- A complete set of intermediate and final outputs can be returned using
  `return_all = TRUE`. This can be useful for downstream analysis; see
  the `Downstream Analysis` section for more.
- Output can be saved to file by specifying a directory to the
  `output_dir` argument. By default, only a .csv file containing the
  node-level meta data and a .pdf file containing the network graph plot
  are saved. Use `save_all = TRUE` to save all output. The filenames
  used for each item can be changed from their default values using the
  arguments `data_outfile`, `plot_outfile`, `cluster_outfile`,
  `igraph_outfile` and `matrix_outfile`. The dimensions (in inches) for
  the pdf of the network graph plot can be adjusted using `plot_width`
  and `plot_height`, with the defaults being `12` and `10`.
- `print_plots = FALSE` can be used to prevent the plots from being
  printed to the `R` plotting window.

## Downstream Analysis

The output returned by `buildRepSeqNetwork()` can be used to facilitate
further downstream analysis, particularly if invoked with
`return_all = TRUE`. Doing so causes the function to return a list
containing the following named components:

- `node_data`: A data frame containing the node-level biological and
  network meta data.
- `cluster_stats`: A data frame containing the cluster-level meta data
  (only included if `cluster_stats = TRUE`).
- `plots`: A list containing all plots generated, with each plot stored
  in as a separate list element named according to the variable used to
  color the nodes. Each plot is a `ggraph` object, which is a special
  kind of `ggplot` object.
- `adjacency_matrix`: The adjacency matrix for the network graph, stored
  as a sparse matrix of class `dgCMatrix` from the `Matrix` package.
- `igraph`: An object of the `igraph` class from the `igraph` package.
  This contains the list of nodes and edges for the network, and can be
  used to generate a `ggraph` plot using the `plotNetworkGraph`
  function.

### Downstream Customization of Plots

While the arguments of `buildRepSeqNetwork()` allow for substantial
customization of the plots generated, you may occasionally find yourself
wanting full control over all aspects of a plot. Or you may want to
modify a plot further downstream based on the initial analysis.

When you use `return_all = TRUE`, all plots generated by
`buildRepSeqNetwork()` are included in the output as `ggraph` objects.
These are special kinds of `ggplot` objects, and their properties can be
manipulated like any plot created with `ggplot()` using the same
functions from the `ggplot2` package.

For example, suppose we have previously run the following:

``` r
output <- buildRepSeqNetwork(tcr_data, "cdr3", node_stats = TRUE, 
                             stats_to_include = "all",
                             return_all = TRUE)
#> Input data contains 4206 rows.
#> Removing sequences with length less than 3... Done. 4206 rows remaining.
#> Computing network edges based on a max hamming distance of 1... Done.
#> 588 nodes in the network (after removing nodes with degree zero).
#> Computing cluster membership within the network... Done.
#> Computing node-level network statistics... Done.
#> Generating graph plot with nodes colored by degree...
```

<img src="man/figures/README-unnamed-chunk-20-1.png" width="100%" style="display: block; margin: auto;" />

    #>  Done.
    #> Finished building network.

Now, suppose we want to remove the plot title/subtitle, adjust the node
size, color the nodes using the `umis` variable and change the color
palette. We can do so using standard `ggplot2` functions (along with the
`geom_node_point` function from the `ggraph` package) as follows:

``` r
library(ggplot2)
output$plots$degree + # modify plot from output
  labs(title = NULL, subtitle = NULL) +  # remove title/subtitle
  ggraph::geom_node_point(  # color nodes by coreness and set node size to 1
    aes(color = output$node_data$umis), size = 1) +
  guides(color = guide_legend(title = "UMIs")) + # change color legend title
  scale_color_gradient(low = "pink", high = "purple4")  # change color gradient
```

<img src="man/figures/README-unnamed-chunk-21-1.png" width="100%" style="display: block; margin: auto;" />

### Generating New Plots

In addition to modifying plots returned by `buildRepSeqNetwork()`, we
also can generate new ones from the `igraph` object included in its
output. This can be simpler when you want a plot that is sufficiently
different from the one in your existing output.

The `igraph` object can be passed to the `plotNetworkGraph` function in
order to generate the plot; this function is a wrapper to the `ggraph`
function, so a plot can also be generated directly using `ggraph()` for
users experienced with `ggplot2`.

Here we generate a new plot, coloring the nodes using the `c_gene`
variable, and manually specifying a color palette using the
`scale_color_manual` function from the `ggplot2` package:

``` r
plotNetworkGraph(network = output$igraph,
                 color_nodes_by = output$node_data$c_gene,
                 color_legend_title = "C Gene",
                 size_nodes_by = 1) +
  scale_color_manual(values = c("grey", "limegreen", "red2"))
```

<img src="man/figures/README-unnamed-chunk-22-1.png" width="100%" style="display: block; margin: auto;" />

### Matching Output Data to Input Data

If one wishes to match rows of the input data to rows of the node-level
meta data returned by `buildRepSeqNetwork()`, this can be done by
matching the row names, since these are preserved in the output data:

``` r
head(output$node_data[ , c("barcode", "cdr3")])
#>               barcode                 cdr3
#> 27 AAACGGGGTGTTCGAT-1        CASSVDRNTEAFF
#> 30 AAACGGGGTTAAAGAC-1        CASKGETNTEAFF
#> 39 AAAGATGAGTCACGCC-1        CASSVGQITEAFF
#> 61 AAAGCAACAAACCCAT-1     CASSQASGGSKNIQYF
#> 65 AAAGCAAGTCCCTACT-1 CASSPPGQGISATLNYGYTF
#> 80 AAAGTAGCATCCTTGC-1   CSAYGTSGEVVQGLTQYF
head(tcr_data[rownames(output$node_data) , c("barcode", "cdr3")])
#>               barcode                 cdr3
#> 27 AAACGGGGTGTTCGAT-1        CASSVDRNTEAFF
#> 30 AAACGGGGTTAAAGAC-1        CASKGETNTEAFF
#> 39 AAAGATGAGTCACGCC-1        CASSVGQITEAFF
#> 61 AAAGCAACAAACCCAT-1     CASSQASGGSKNIQYF
#> 65 AAAGCAAGTCCCTACT-1 CASSPPGQGISATLNYGYTF
#> 80 AAAGTAGCATCCTTGC-1   CSAYGTSGEVVQGLTQYF
```

### Generating `igraph` Network From Adjacency Matrix

It is not necessary to save the `igraph` object for a network as long as
one has the adjacency matrix returned by `buildRepSeqNetwork()`.
Regenerating the `igraph` object from the adjacency matrix is extremely
fast, and can be done using the `generateNetworkFromAdjacencyMat`
function, which is a convenient wrapper to the `igraph` functions
`graph_from_adjacency_matrix`, `simplify` and `as.undirected`.

``` r
network <- generateNetworkFromAdjacencyMat(output$adjacency_matrix)
```

This object can then be passed to `plotNetworkGraph()` or other
functions that act on `igraph` network objects.

### Generating `igraph` Network From TCR/BCR Sequences

If the adjacency matrix is unavailable and one wishes to regenerate the
`igraph` network object directly from the TCR/BCR sequences without
calling `buildRepSeqNetwork()` again, this can be done using the
`generateNetworkFromSeqs` function. The column of the node-level meta
data containing the TCR/BCR sequences is passed to the first argument:

``` r
network <- generateNetworkFromSeqs(output$node_data$cdr3)
```

While this involves computing the adjacency matrix, it can be
considerably faster than using `buildRepSeqNetwork()` again with the
original input data: assuming `buildRepSeqNetwork()` was originally
called with the default setting of `drop_isolated_nodes = TRUE`, then
depending on the number of nodes dropped, the number of pairwise
distances that must be recomputed could potentially be far less than the
number originally computed.

### Computing Network Properties

Node-level network properties can be computed and added to the
node-level meta data using the `igraph` network object:

``` r
output <- buildRepSeqNetwork(tcr_data, "cdr3", 
                             return_all = TRUE, print_plots = FALSE)

output$node_data <- addNodeNetworkStats(output$node_data,
                                        net = output$igraph,
                                        stats_to_include = "all")
```

Cluster-level properties can be computed using the node-level meta data
and the adjacency matrix:

``` r
output$cluster_stats <- 
  getClusterStats(data = output$node_data,
                  adjacency_matrix = output$adjacency_matrix,
                  seq_col = "cdr3")
```

### Computing Adjacency Matrices

If one wishes to regenerate the adjacency matrix for a network, this can
be done by passing the column of the node-level meta data containing the
TCR/BCR sequences to the `sparseAdjacencyMatFromSeqs` function. This
function is also useful in general as a computationally fast and memory
efficient tool for generating graph adjacency matrices based on the
Hamming or Levenshtein distance, as the matrices are computed in C++ as
sparse integer matrices and returned in `R` as sparse matrices of class
`dgCMatrix` from the `Matrix` package.

``` r
adjacency_matrix <- sparseAdjacencyMatFromSeqs(output$node_data$cdr3)
```

# Finding Associated Clones

Vignette content coming soon.

# Finding Public Clones

Vignette content coming soon.
