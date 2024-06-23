#include <stdio.h>
#include <iostream>
#include <fstream>
#include <memory>
#include <string.h>
#include <vector>
#include <list>
#include <expected>
#include <ranges>
#include <charconv>
#include <algorithm>
#include "string_tokenizer.hpp"

enum class graph_error
{
    node_not_found,
    lenght_parsing_error,
};

struct Connection;
struct Node;

struct Node
{
    std::string_view name;
    long id{0};
    int weight{0};
    std::vector<std::reference_wrapper<Connection>> connections{};

    bool visited{false};
    std::vector<std::vector<std::reference_wrapper<Connection>>> paths{};

    Node(std::string_view name, long id) : name(name), id(id), weight(INT_MAX)
    {
    }

    ~Node()
    {
        clear_paths();
    }

    bool operator==(const Node &other) const
    {
        return this->id == other.id;
    }

    void clear_paths()
    {
        for (auto &&path : paths)
            path.clear();
        paths.clear();
    }
};

struct Connection
{
    Node &from; // all nodes are created before connections, so memory are not moving, -> refs are ok
    Node &to;
    int lenght;

    Connection(Node &from, Node &to, int len) : from(from), to(to), lenght(len) {}

    Node &opposite(Node &node) const
    {
        if (node == from)
            return to;
        return from;
    }
};

void print_path(const std::vector<std::reference_wrapper<Connection>> &path);

class Graph
{
public:
    Graph(std::string_view content)
    {
        this->buffer = std::make_unique<char[]>(content.length() + 1); // string_view are not null terminated, strncpy makes null terminated string
        strncpy(buffer.get(), content.data(), content.length());
    }

private:
    std::unique_ptr<char[]> buffer; // make a copy of graph content, not to alloc each node name
    long node_id_seq{0};
    bool needs_reset{false};
    std::vector<Node> nodes;
    std::vector<Connection> connections;

    std::optional<std::reference_wrapper<Node>> find_node(std::string_view name)
    {
        auto &&iter = std::ranges::find_if(nodes, [&](const Node &node)
                                           { return node.name.compare(name) == 0; });

        if (iter == nodes.end())
            return std::nullopt;
        return *iter;
    }

    void create_node(std::string_view name)
    {
        auto &&iter = std::ranges::find_if(nodes, [&](const Node &node)
                                           { return node.name.compare(name) == 0; });

        if (iter != nodes.end())
            return;

        nodes.emplace_back(name, ++node_id_seq);
    }

    void parse_nodes(const std::string_view &line)
    {
        std::string_view name1;
        std::string_view name2;

        auto tokenizer{StringTokenizer(line, ':')};

        auto i{0};
        while (tokenizer.has_next() && i != 2)
        {
            auto data = tokenizer.next();
            switch (i)
            {
            case 0:
                name1 = data;
                break;
            case 1:
                name2 = data;
                break;
            default:
                break;
            }
            ++i;
        }

        if (i != 2)
            return;

        create_node(name1);
        create_node(name2);
    }

    std::optional<graph_error> parse_line(std::string_view line)
    {
        std::string_view name1;
        std::string_view name2;
        int length{0};

        auto tokenizer{StringTokenizer(line, ':')};

        auto i{0};
        while (tokenizer.has_next())
        {
            auto data = tokenizer.next();
            switch (i)
            {
            case 0:
                name1 = data;
                break;
            case 1:
                name2 = data;
                break;
            case 2:
                try
                {
                    std::from_chars(data.data(), data.data() + data.size(), length);
                }
                catch (...)
                {
                    return graph_error::lenght_parsing_error;
                }
                break;
            default:
                break;
            }
            ++i;
        }

        if (i != 3)
            return std::nullopt;

        auto node1_opt = find_node(name1);
        auto node2_opt = find_node(name2);
        if (!node1_opt.has_value() || !node2_opt.has_value())
            return graph_error::node_not_found;

        auto &node1 = node1_opt.value().get();
        auto &node2 = node2_opt.value().get();

        connections.emplace_back(node1, node2, length);

        return std::nullopt;
    }

    void reset()
    {
        for (auto &node : nodes)
        {
            node.visited = false;
            node.weight = INT_MAX;
            for (auto &route : node.paths)
                route.clear();
            node.paths.clear();
        }
    }

    void process_node(Node &from_node)
    {
        std::list<std::reference_wrapper<Node>> unvisited;
        from_node.visited = true;

        for (auto &conn_ref : from_node.connections)
        {
            auto &conn = conn_ref.get();
            auto &opposite_node = conn.opposite(from_node);

            if (!opposite_node.visited)
                unvisited.push_front(opposite_node);

            const auto lenghtTo = from_node.weight + conn.lenght;

            if (lenghtTo <= opposite_node.weight)
            {
                if (lenghtTo < opposite_node.weight)
                {
                    opposite_node.weight = lenghtTo;
                    opposite_node.clear_paths();
                }

                for (const auto &route : from_node.paths)
                {
                    auto new_route = std::vector(route);
                    new_route.emplace_back(conn);

                    opposite_node.paths.emplace_back(std::move(new_route));
                }
            }
        }

        while (!unvisited.empty())
        {
            process_node(unvisited.back());
            unvisited.pop_back();
        }
    }

    void print_node(const Node &node)
    {
        std::cout << "Node: " << node.name << '\n';
        std::cout << "Connections: " << '\n';
        for (auto &&conn_ref : node.connections)
        {
            std::cout << '\t' << conn_ref.get().from.name << " -> " << conn_ref.get().to.name << '\n';
        }
    }

public:
    std::expected<std::vector<std::vector<std::reference_wrapper<Connection>>>, graph_error> find_paths(std::string_view from, std::string_view to)
    {
        auto from_node_opt = find_node(from);
        auto to_node_opt = find_node(to);

        if (!from_node_opt.has_value() || !to_node_opt.has_value())
            return std::unexpected(graph_error::node_not_found);

        auto &from_node = from_node_opt.value().get();
        auto &to_node = to_node_opt.value().get();

        if (needs_reset) {
            reset();
            needs_reset = false;
        }

        from_node.visited = true;
        from_node.weight = 0;

        from_node.paths.emplace_back();

        process_node(from_node);

        needs_reset = true;

        return to_node.paths;
    }

    void print_graph()
    {
        for (auto &node : nodes)
            print_node(node);
    }

    static std::expected<Graph, graph_error> parse_graph(std::string_view raw_content)
    {
        Graph graph(raw_content);
        auto tokenizer{StringTokenizer(graph.buffer.get(), '\n')};

        while (tokenizer.has_next())
        {
            auto line = tokenizer.next();
            if (line.empty())
                continue;

            graph.parse_nodes(line);
        }
        tokenizer.reset();
        while (tokenizer.has_next())
        {
            auto line = tokenizer.next();
            if (line.empty())
                continue;

            auto error = graph.parse_line(line);
            if (error.has_value())
                return std::unexpected(error.value());
        }

        for (auto &node : graph.nodes)
        {
            auto nodeConns = graph.connections | std::views::filter([&](const Connection &conn)
                                                                    { return conn.from.id == node.id || conn.to.id == node.id; });

            for (auto &conn : nodeConns)
                node.connections.emplace_back(std::ref(conn));
        }

        return graph;
    }
};

void print_path(const std::vector<std::reference_wrapper<Connection>> &path)
{
    for (auto &conn : path)
        std::cout << conn.get().from.name << " -> " << conn.get().to.name << " ";

    std::cout << '\n';
}

int main(int, char **)
{
    std::string filePath;
    std::cout << "Enter filepath\n";
    std::cin >> filePath;

    std::ifstream fileStream(filePath);

    const auto BUFFER_SIZE = 256;
    const auto DELIMITER = "\n";
    auto buffer = std::make_unique<char[]>(BUFFER_SIZE);

    fileStream.read(buffer.get(), BUFFER_SIZE);

    auto graph_or_err = Graph::parse_graph(buffer.get());

    if (!graph_or_err.has_value())
    {
        std::cout << "Graph parsing error\n";
        return 1;
    }

    auto &graph = graph_or_err.value();
    graph.print_graph();

    std::string from, to;
    std::cin >> from >> to;
    std::cout << "working\n";

    auto res = graph.find_paths(from, to);
    if (res.has_value())
    {
        auto &result = res.value();
        std::cout << "Found: " << result.size() << std::endl;

        for (auto &&i : result)
        {
            print_path(i);
        }
    }

    return 0;
}
