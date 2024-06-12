# Open the file and fill the data list with a line per element
with open("output.txt") as f:
    data = f.read().split("\n")
    # If the final entry is empty remove it
    if data[-1] == "":
        data.pop()

# This var will remain true if it is always the case that 
# iff the point is inside the triangle, then 
# alpha and beta are possitive and alpha + beta is in the range 0-1 inclusive
assertion = True

# For each data entry
for d in data:
    # Seperate the values
    inside, alpha, beta = d.split(":")
    # Conver the coordinates to numbers (from string)
    alpha,beta = float(alpha),float(beta)
    if inside == "True":
        
        if not (alpha >= 0 and beta >= 0 and alpha+beta <= 1):
            # The assertion must be false
            assertion = False
            # Output the entry that contradicted the assertion
            print(d)
    # Check that the reverse of the assertion also holds
    elif alpha >= 0 and beta >= 0 and alpha+beta <= 1:
        assertion = False
        print(d)
    
print(assertion)