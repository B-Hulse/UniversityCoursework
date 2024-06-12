# Initialization from a BayesianModel object
from pgmpy.factors.discrete import TabularCPD
from pgmpy.models import BayesianModel
from pgmpy.inference import VariableElimination
from GibbsSamplingWithEvidence import GibbsSampling
import numpy as np
import argparse
import re
import math

# This parses the inputs from test_asia.py (You don't have to modify this!)

parser = argparse.ArgumentParser(description='Asia Bayesian network')
parser.add_argument('--evidence', nargs='+', dest='eVars')
parser.add_argument('--query', nargs='+', dest='qVars')
parser.add_argument('-N', action='store', dest='N')
parser.add_argument('--exact', action="store_true", default=False)
parser.add_argument('--gibbs', action="store_true", default=False)
parser.add_argument('--ent', action="store_true", default=False)

args = parser.parse_args()

print('\n-----------------------------------------------------------')

evidence={}
for item in args.eVars:
    evidence[re.split('=|:', item)[0]]=int(re.split('=|:',item)[1])

print('evidence:', evidence)

query = args.qVars
print('query:', args.qVars)

if args.N is not None:
    N = int(args.N)


# Using TabularCPD, define CPDs

cpd_asia = TabularCPD(
    variable='asia', 
    variable_card=2,
    values=[[0.99],[0.01]]
)
cpd_smoke = TabularCPD(
    variable='smoke', 
    variable_card=2,
    values=[[0.5],[0.5]]
)
cpd_tub = TabularCPD(
    variable='tub', 
    variable_card=2,
    values=[[0.99,0.95],[0.01,0.05]],
    evidence=['asia'],
    evidence_card=[2]
)
cpd_bron = TabularCPD(
    variable='bron', 
    variable_card=2,
    values=[[0.7,0.4],[0.3,0.6]],
    evidence=['smoke'],
    evidence_card=[2]
)
cpd_either = TabularCPD(
    variable='either', 
    variable_card=2,
    values=[[0.999,0.001,0.001,0.001],[0.001,0.999,0.999,0.999]],
    evidence=['lung','tub'],
    evidence_card=[2,2]
)
cpd_lung = TabularCPD(
    variable='lung', 
    variable_card=2,
    values=[[0.99,0.9],[0.01,0.1]],
    evidence=['smoke'],
    evidence_card=[2]
)
cpd_xray = TabularCPD(
    variable='xray', 
    variable_card=2,
    values=[[0.95,0.02],[0.05,0.98]],
    evidence=['either'],
    evidence_card=[2]
)
cpd_dysp = TabularCPD(
    variable='dysp', 
    variable_card=2,
    values=[[0.9,0.2,0.3,0.1],[0.1,0.8,0.7,0.9]],
    evidence=['either','bron'],
    evidence_card=[2,2]
)

# Define edges of the asia model

asia_model = BayesianModel([
    ('asia','tub'),
    ('tub','either'),
    ('smoke','lung'),
    ('smoke','bron'),
    ('lung','either'),
    ('bron','dysp'),
    ('either','xray'),
    ('either','dysp')
])

# Associate the parameters with the model structure.

asia_model.add_cpds(cpd_asia,cpd_smoke,cpd_tub, cpd_bron, cpd_either, cpd_lung,cpd_xray,cpd_dysp)

# Find exact solution if args.exact is True:

# A Dictionary to store the exact probabilities of each entry of 'query'
# Format {x:p(x)}
px_vals = {}

def find_exact():
    """
    Computes the exact probabilities of each element in 'query' using Variable Elimination
    Places the probability for each entry of 'query' into the dictionary 'px_vals'
    """
    print("Variable Elimination")
    # Initialise the inference for 'asia_model'
    asia_infer = VariableElimination(asia_model)
    # Calculate, store and print the probability for each item in 'query'
    for x in query:
        px = asia_infer.query([x],evidence=evidence)
        px_vals[x] = px.values[1]
        print("  ",x,": ",px.values[1])

# if '--exact' is included as a command line argument
if args.exact:
    find_exact()

# Find approximate solution and cross entropy if args.gibbs is True:

# A Dictionary to store the approximated probabilities of each entry of query
# Format {x:q(x)}
qx_vals = {}

def find_approx():
    """
    Computes the approximate probability for each entry in 'query' using Gibbs Sampling
    Places the probability for each entry of 'query' into the dictionary 'px_vals'
    """
    print("Gibbs Sampling N =",N)
    asia_sampler = GibbsSampling(asia_model)
    # Create the Gibbs Sampling table
    qs = asia_sampler.sample(size=N,evidence=evidence)
    print("QS",qs)
    # Calculate, store and print the probability for each item in 'query'
    for x in query:
        # Sum the column associated with the query:x and divide by the size:N
        qx = sum(qs[x]) / N
        qx_vals[x] = qx 
        print("  ",x,": ",qx)

# if '--gibbs' is included as a command line argument
if args.gibbs:
    find_approx()

def cross_entropy():
    """
    Calculates the cross-entropy between the exact and approximate probabilities 
    stored in 'px_vals' and 'qx_vals' respectively
    Prints the result
    """
    sum = 0
    for x in query:
        # Get the value for p(x), p(not x), q(x) and q(not x)
        px = px_vals[x]
        npx = 1 - px
        qx = qx_vals[x]
        nqx = 1- qx
        # Calculate the value for this x and add to the sum
        val = (nqx*np.log10(npx)) + (qx*np.log10(px))
        sum -= val
    # Negate the sum to find the final value of cross_entropy
    cross_entropy = sum

    print("Cross-Entropy: ",cross_entropy)

# if '--ent' is included as a command line argument
if args.ent:
    # Checks that both exact and approx values have been calculated, as they are required for cross-entropy
    if not (args.gibbs or args.exact):
        print("Cannot calculate cross entropy without finding both exact and gibbs sampling solutions")
    else:
        cross_entropy()

print('-----------------------------------------------------------')
