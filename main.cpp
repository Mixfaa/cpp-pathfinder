#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string.h>
#include <vector>
#include <list>
#include <forward_list>
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

void print_path(const std::vector<std::reference_wrapper<Connection>> &path);
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
    Node &from;
    Node &to;
    int lenght;

    Connection(Node &from, Node &to, int len) : from(from), to(to), lenght(len) {}

    Node &opposite(Node &node) const
    {
        return node == from ? to : from;
    }
};

bool are_routes_same(const std::vector<std::reference_wrapper<Connection>> &route1,
                     const std::vector<std::reference_wrapper<Connection>> &route2)
{
    if (std::addressof(route1) == std::addressof(route2))
        return true;
    if (route1.size() != route2.size())
        return false;

    for (size_t i = 0; i < route1.size(); i++)
    {
        const auto &connection1 = route1[i].get();
        const auto &connection2 = route2[i].get();

        if (std::addressof(connection1) != std::addressof(connection2))
            return false;
    }

    return true;
}

bool paths_have_route(
    const std::vector<std::vector<std::reference_wrapper<Connection>>> &paths,
    const std::vector<std::reference_wrapper<Connection>> &this_route,
    const Connection &route_tail)
{
    for (auto &route : paths)
    {
        if (route.size() != (this_route.size() + 1))
            continue;

        for (size_t i = 0; i < this_route.size(); i++)
        {
            const auto &connection1 = this_route[i].get();
            const auto &connection2 = route[i].get();
            if (std::addressof(connection1) != std::addressof(connection2))
                break;
        }

        if (std::addressof(route.back().get()) == std::addressof(route_tail))
            return true;
    }

    return false;
}

void print_path(const std::vector<std::reference_wrapper<Connection>> &path);

class Graph
{
public:
    Graph(std::string_view content)
    {
        this->buffer = std::make_unique<char[]>(content.length() + 2); // string_view are not null terminated, strncpy makes null terminated string
        strncpy(buffer.get(), content.data(), content.length());
    }

private:
    std::unique_ptr<char[]> buffer; // make a copy of graph content, not to alloc each node name
    long node_id_seq{0};
    bool needs_reset{false};
    std::forward_list<Node> nodes; // forward list does not invalidate references and iterators
    std::forward_list<Connection> connections;

    std::optional<std::reference_wrapper<Node>> find_node(const std::string_view &name)
    {
        auto &&iter = std::ranges::find_if(nodes, [&](const Node &node)
                                           { return node.name.compare(name) == 0; });

        if (iter == nodes.end())
            return std::nullopt;
        return *iter;
    }

    Node &find_create_node(std::string_view name)
    {
        auto &&iter = std::ranges::find_if(nodes, [&](const Node &node)
                                           { return node.name.compare(name) == 0; });

        return (iter == nodes.end()) ? nodes.emplace_front(name, ++node_id_seq) : *iter;
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

        connections.emplace_front(find_create_node(name1), find_create_node(name2), length);

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
                    if (!paths_have_route(opposite_node.paths, route, conn))
                    {
                        auto new_route = std::vector(route);
                        new_route.emplace_back(conn);

                        opposite_node.paths.emplace_back(std::move(new_route));
                    }
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

        if (needs_reset)
        {
            reset();
            needs_reset = false;
        }

        from_node.visited = true;
        from_node.weight = 0;

        from_node.paths.emplace_back();

        process_node(from_node);

        needs_reset = true;

        auto &paths = to_node.paths;

        // paths.erase(
        //     std::unique(paths.begin(), paths.end(), are_routes_same), paths.end());

        return std::move(paths);
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

            graph.parse_line(line);
        }

        for (auto &node : graph.nodes)
        {
            auto nodeConns = graph.connections | std::views::filter([&](const Connection &conn)
                                                                    { return conn.from.id == node.id || conn.to.id == node.id; });

            for (auto &conn : nodeConns)
                node.connections.emplace_back(conn);
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
