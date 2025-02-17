import csv
from neo4j import GraphDatabase

class Neo4jApp:
    def __init__(self, uri, user, password):
        self.driver = GraphDatabase.driver(uri, auth=(user, password))

    def close(self):
        self.driver.close()

    def create_edge(self, from_node, to_node, relationship):
        with self.driver.session() as session:
            session.write_transaction(self._create_and_return_edge, from_node, to_node, relationship)

    @staticmethod
    def _create_and_return_edge(tx, from_node, to_node, relationship):
        query = (
                "MERGE (a:Node {name: $from_node}) "
                "MERGE (b:Node {name: $to_node}) "
                "MERGE (a)-[r:" + relationship + "]->(b) "
                                                 "RETURN a, b, r"
        )
        result = tx.run(query, from_node=from_node, to_node=to_node)
        return result.single()

    def run_query(self, query):
        with self.driver.session() as session:
            result = session.run(query)
            return [record for record in result]

    def delete_all(self):
        with self.driver.session() as session:
            session.write_transaction(self._delete_all)

    @staticmethod
    def _delete_all(tx):
        tx.run("MATCH (n) DETACH DELETE n")

    def load_edges_from_csv(self, csv_file_path):
        with open(csv_file_path, 'r') as file:
            reader = csv.reader(file)
            for row in reader:
                from_node, to_node, relationship, _ = row
                self.create_edge(from_node, to_node, relationship)

if __name__ == "__main__":
    app = Neo4jApp("bolt://localhost:7687", "neo4j", "password")

    # Erase current data
    app.delete_all()

    # Load graph edges from CSV file
    app.load_edges_from_csv('../../dataset/so-stream_debug_5kk_regex.csv')

    query = """
            MATCH (start)-[:a]->(middle)
            OPTIONAL MATCH (middle)-[:b*0..]->(end)
            RETURN COUNT(DISTINCT end) AS distinct_count
            """
    results = app.run_query(query)

    for record in results:
        print(record["distinct_count"])
        results = app.run_query(query)

    app.close()