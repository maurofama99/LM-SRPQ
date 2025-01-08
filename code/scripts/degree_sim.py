import math

def compute_decayed_expiration(expiration, current_time, gamma, lamb=5):
    """
    Compute the decayed expiration time based on the formula:
    DecayedExpiration = Expiration * e^(-gamma * (current_time - Expiration))
    """
    # print("exp: " + (math.exp(-gamma * (current_time - expiration))).__str__())
    if current_time == expiration : return current_time
    else: return lamb * math.exp(-gamma * (1 / (expiration-current_time)))

def compute_weighted_mean_expiration(neighbors_expirations, current_time, gamma):
    """
    Compute the weighted mean of expiration times from neighbors, considering decay.
    WeightedExpiration = Σ(w_i * DecayedExpiration_i) / Σ(w_i)
    where w_i = e^(-gamma * (current_time - expiration_i))
    """
    if not neighbors_expirations:
        return current_time  # No neighbors, return current time
    weighted_sum = 0
    weight_sum = 0
    # CONSIDER ALL NEIGHBORS
    # for exp in neighbors_expirations:
        # Compute the weight based on time decay, which simplifies to 1 as arrival time is current_time
    #    weight = compute_decayed_expiration(exp, current_time, gamma)
    #    weighted_sum += weight * exp
    #    weight_sum += weight
    #return weighted_sum / weight_sum if weight_sum > 0 else current_time

    return compute_decayed_expiration(max(neighbors_expirations),current_time,gamma)



def compute_theta(degree_v, degree_u, alpha=1.5):
    """
    Compute theta with degree scaling using the formula:
    θ(v) = δ(v)^α / (δ(v)^α + δ(u)^α)
    θ(u) = 1 - θ(v)
    """
    weighted_v = degree_v ** alpha
    weighted_u = degree_u ** alpha
    theta_v = weighted_v / (weighted_v + weighted_u)
    theta_u = 1 - theta_v
    return theta_v, theta_u



def compute_edge_expiration(theta_v, theta_u, weighted_exp_v, weighted_exp_u, base_offset=5):
    """
    Compute the contribution factor and expiration timestamp of an edge.
    """
    contribution_factor = (theta_v * weighted_exp_v + theta_u * weighted_exp_u)
    return base_offset + contribution_factor

def main():
    # Input
    current_time = float(input("Enter the current timestamp: "))
    gamma = 0.01

    print("For vertex v:")
    neighbors_v = int(input("Enter the number of neighbors for v: "))
    expiration_times_v = [
        float(input(f"Enter expiration time for neighbor {i + 1}: "))
        for i in range(neighbors_v)
    ]
    degree_v = neighbors_v  # Assuming degree equals the number of neighbors

    print("For vertex u:")
    neighbors_u = int(input("Enter the number of neighbors for u: "))
    expiration_times_u = [
        float(input(f"Enter expiration time for neighbor {i + 1}: "))
        for i in range(neighbors_u)
    ]
    degree_u = neighbors_u  # Assuming degree equals the number of neighbors

    # Compute weighted mean decayed expiration times using current timestamp as arrival time
    weighted_exp_v = compute_weighted_mean_expiration(expiration_times_v, current_time, gamma)
    weighted_exp_u = compute_weighted_mean_expiration(expiration_times_u, current_time, gamma)
    print("v: " + weighted_exp_v.__str__())
    print("u: " + weighted_exp_u.__str__())

    # Compute theta values dynamically
    theta_v, theta_u = compute_theta(degree_v, degree_u)
    print(f"Theta for vertex v: {theta_v:.2f}, Theta for vertex u: {theta_u:.2f}")

    # Compute expiration timestamp for the edge
    expiration_time = compute_edge_expiration(theta_v, theta_u, weighted_exp_v, weighted_exp_u)
    print(f"The expiration timestamp for the edge is: {expiration_time:.2f}")

if __name__ == "__main__":
    main()
