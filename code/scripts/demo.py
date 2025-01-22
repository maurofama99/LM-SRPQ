import math
import random
from collections import defaultdict

class Window:
    def __init__(self, id, opening_time):
        self.id = id
        self.opening_time = opening_time
        self.expiration_time = opening_time
        self.last_update = 0
        self.edges = []  # Store edges assigned to this window
        self.vertices = set()  # Store vertices assigned to this window

    def update_expiration(self, lambda_, gamma):
        """
        Update the expiration time based on the current step `last_update`.
        """
        self.expiration_time = lambda_ * math.exp(-gamma / self.last_update) + self.opening_time

    def add_edge(self, edge):
        if not self.edges:  # If this is the first edge
            self.opening_time = edge.timestamp
        self.edges.append(edge)
        self.vertices.add(edge.v)
        self.vertices.add(edge.u)
        self.last_update += 1  # Increment the step of advancement

    def __str__(self):
        return (f"Window {self.id} | Opening Time: {self.opening_time}, "
                f"Expiration Time: {self.expiration_time}, Last Update: {self.last_update}, "
                f"Edges: {self.edges}")

class Edge:
    def __init__(self, v, u, label, timestamp):
        self.v = v
        self.u = u
        self.label = label
        self.timestamp = timestamp

    def __repr__(self):
        return f"Edge({self.v}-{self.u}, label={self.label}, ts={self.timestamp})"

    def __str__(self):
        return f"Edge({self.v}-{self.u}, label={self.label}, ts={self.timestamp})"

class SnapshotGraph:
    def __init__(self):
        self.graph = defaultdict(set)
        self.timestamps = {}  # Store the timestamp of each vertex

    def add_edge(self, edge):
        """
        Add an edge to the graph and update degrees.
        """
        self.graph[edge.v].add(edge.u)
        self.graph[edge.u].add(edge.v)
        # Assign timestamps to vertices if not already assigned
        #if edge.v not in self.timestamps:
        self.timestamps[edge.v] = edge.timestamp
        #if edge.u not in self.timestamps:
        self.timestamps[edge.u] = edge.timestamp

    def get_degree(self, vertex):
        """
        Get the degree of a vertex.
        """
        return len(self.graph[vertex])

    def draw_ascii(self):
        """
        Render the graph as ASCII art with vertex timestamps.
        """
        for node, neighbors in self.graph.items():
            # Add timestamp to the vertex name
            timestamp = self.timestamps[node]
            formatted_node = f"{node}t{timestamp}"
            connections = ", ".join([f"{neighbor}t{self.timestamps[neighbor]}" for neighbor in neighbors])
            print(f"{formatted_node}: [{connections}]")

    def __str__(self):
        return "\n".join([f"{node}: {neighbors}" for node, neighbors in self.graph.items()])

def compute_theta(degree_v, degree_u, alpha):
    """
    Compute theta for vertices v and u based on their degrees.
    θ(v) = δ(v)^α / (δ(v)^α + δ(u)^α)
    """
    theta_v = (degree_v ** alpha) / ((degree_v ** alpha) + (degree_u ** alpha))
    theta_u = 1 - theta_v
    return theta_v, theta_u

def assignWindow(edge, snapshot, windows, alpha, lambda_, gamma, beta):
    """
    Assign a window to an edge based on the dynamic expiration calculation.

    Parameters:
    - edge (Edge): The edge being processed.
    - snapshot (SnapshotGraph): The current graph structure for degree computations.
    - windows (dict): A dictionary of active windows indexed by their IDs.

    Returns:
    - float: The expiration time for the edge.
    """
    # Compute degrees and theta
    delta_v = snapshot.get_degree(edge.v)
    delta_u = snapshot.get_degree(edge.u)
    theta_v = (delta_v ** alpha) / ((delta_v ** alpha) + (delta_u ** alpha))
    theta_u = 1 - theta_v

    # Select window based on theta
    if theta_v > theta_u:
        window_id = edge.v
    else:
        window_id = edge.u

    # Get or create the corresponding window
    if window_id not in windows:
        windows[window_id] = Window(id=window_id, opening_time=edge.timestamp)
    window = windows[window_id]

    # Update snapshot with the new edge
    snapshot.add_edge(edge)

    # Calculate expiration time
    n = window.last_update + 1
    t_open = window.opening_time
    new_exp = lambda_ * math.exp(-gamma / n) + t_open

    # Adjust lambda if the expiration time is invalid
    if new_exp < edge.timestamp:
        lambda_adjustment = (edge.timestamp - new_exp) * beta
        lambda_ += lambda_adjustment
        new_exp = lambda_ * math.exp(-gamma / n) + t_open

    # Update window metadata
    window.expiration_time = new_exp
    window.last_update = n
    window.add_edge(edge)

    return new_exp

def print_window_vertices(windows):
    """
    Print the vertices that belong to each window.
    """
    print("\nVertices in Each Window:")
    for window_id, window in windows.items():
        print(f"Window {window_id} includes vertices: {', '.join(window.vertices)}")

def simulate_stream(num_edges, max_vert, alpha, lambda_, gamma, beta):
    """
    Simulate a stream of edges and process them into windows.
    """
    # Initialize snapshot graph and windows
    graph = SnapshotGraph()
    windows = defaultdict(lambda: Window(id=None, opening_time=0))

    # Generate a random stream of edges
    edges = [
        Edge(v=f"v{random.randint(1, max_vert)}", u=f"v{random.randint(1, max_vert)}",
             label=f"l{i}", timestamp=1217801240 + i * random.randint(1, 60))
        for i in range(1, num_edges + 1)
    ]


    print("Simulating Stream of Edges:")
    for edge in edges:
        print(edge)

    print("\nProcessing Stream:")
    for edge in edges:
        # Add edge to the snapshot graph
        graph.add_edge(edge)

        # Assign window and compute expiration
        exp_time = assignWindow(edge, graph, windows, alpha, lambda_, gamma, beta)
        print(f"Processed {edge}, Expiration Time: {exp_time:.2f}")

    # Print the vertices in each window
    # print_window_vertices(windows)

    print("\nFinal State of Windows:")
    # for window in windows.values():
    # print(window)

    print("\nFinal Snapshot Graph:")
    # graph.draw_ascii()


if __name__ == "__main__":
    # Parameters for Unix timestamp-based simulation
    n_edges = 50000  # Number of edges in the stream
    a = 1.5  # Exponent for degree influence
    l = 172800  # Base amplitude for expiration (2 days in seconds)
    g = 0.01  # Decay rate
    b = 2.5  # Lambda adjustment factor
    max_v = 40000  # Maximum number of vertices

    simulate_stream(n_edges, max_v, a, l, g, b)

