#include "Mesh.hpp"
#include "read_write_chunk.hpp"

#include <glm/glm.hpp>

#include <stdexcept>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <cstddef>

MeshBuffer::MeshBuffer(std::vector<std::string> const &filename) {
	glGenBuffers(1, &buffer);

	std::vector<int> data_index(1, 0), str_index(1, 0);
	int data_index_i = 0, str_index_i = 0;

	std::vector<std::ifstream> file;
	for (auto &fn : filename) {
	    file.emplace_back(fn, std::ios::binary);
	}

//	GLuint total = 0;

	struct Vertex {
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::u8vec4 Color;
		glm::vec2 TexCoord;
	};
	static_assert(sizeof(Vertex) == 3*4+3*4+4*1+2*4, "Vertex is packed.");
	std::vector< Vertex > data, temp;

	//read + upload data chunk:
	for (auto &f : file) {
        read_chunk(f, "pnct", &temp);
        data_index.push_back(temp.size());
        data.insert(data.end(), temp.begin(), temp.end());
	}

    //upload data:
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(Vertex), data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

//    total = GLuint(data.size()); //store total for later checks on index

    //store attrib locations:
    Position = Attrib(3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, Position));
    Normal = Attrib(3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, Normal));
    Color = Attrib(4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), offsetof(Vertex, Color));
    TexCoord = Attrib(2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, TexCoord));



	std::vector< char > strings, temp2;

    for (auto &f : file) {
        read_chunk(f, "str0", &temp2);
        str_index.push_back(temp2.size());
        strings.insert(strings.end(), temp2.begin(), temp2.end());
    }

	{ //read index chunk, add to meshes:
		struct IndexEntry {
			uint32_t name_begin, name_end;
			uint32_t vertex_begin, vertex_end;
		};
		static_assert(sizeof(IndexEntry) == 16, "Index entry should be packed");

		for (auto& f : file) {
            std::vector< IndexEntry > index;
            read_chunk(f, "idx0", &index);

            data_index_i += data_index.front();
            str_index_i += str_index.front();
            data_index.erase(data_index.begin());
            str_index.erase(str_index.begin());

            for (auto const &entry : index) {
//                if (!(entry.name_begin <= entry.name_end && entry.name_end <= strings.size())) {
//                    throw std::runtime_error("index entry has out-of-range name begin/end");
//                }
//                if (!(entry.vertex_begin <= entry.vertex_end && entry.vertex_end <= total)) {
//                    throw std::runtime_error("index entry has out-of-range vertex start/count");
//                }
                std::string name(&strings[0] + entry.name_begin + str_index_i, &strings[0]
                + entry.name_end + str_index_i);
//                std::cout << name << std::endl;
                Mesh mesh;
                mesh.type = GL_TRIANGLES;
                mesh.start = entry.vertex_begin + data_index_i;
                mesh.count = entry.vertex_end - entry.vertex_begin;
                for (uint32_t v = entry.vertex_begin + data_index_i; v <
                entry.vertex_end + data_index_i;
                ++v) {
                    mesh.min = glm::min(mesh.min, data[v].Position);
                    mesh.max = glm::max(mesh.max, data[v].Position);
                }
                bool inserted = meshes.insert(std::make_pair(name, mesh)).second;
                if (!inserted) {
                    std::cout << "WARNING: mesh name '" + name + " collides "
                                                                 "with existing mesh." << std::endl;
                }
            }
		}
	}

//	if (file.peek() != EOF) {
//		std::cerr << "WARNING: trailing data in mesh file '" << filename << "'" << std::endl;
//	}

	/* //DEBUG:
	std::cout << "File '" << filename << "' contained meshes";
	for (auto const &m : meshes) {
		if (&m.second == &meshes.rbegin()->second && meshes.size() > 1) std::cout << " and";
		std::cout << " '" << m.first << "'";
		if (&m.second != &meshes.rbegin()->second) std::cout << ",";
	}
	std::cout << std::endl;
	*/
}

const Mesh &MeshBuffer::lookup(std::string const &name) const {
	auto f = meshes.find(name);
	if (f == meshes.end()) {
		throw std::runtime_error("Looking up mesh '" + name + "' that doesn't exist.");
	}
	return f->second;
}

GLuint MeshBuffer::make_vao_for_program(GLuint program) const {
	//create a new vertex array object:
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//Try to bind all attributes in this buffer:
	std::set< GLuint > bound;
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	auto bind_attribute = [&](char const *name, MeshBuffer::Attrib const &attrib) {
		if (attrib.size == 0) return; //don't bind empty attribs
		GLint location = glGetAttribLocation(program, name);
		if (location == -1) return; //can't bind missing attribs
		glVertexAttribPointer(location, attrib.size, attrib.type, attrib.normalized, attrib.stride, (GLbyte *)0 + attrib.offset);
		glEnableVertexAttribArray(location);
		bound.insert(location);
	};
	bind_attribute("Position", Position);
	bind_attribute("Normal", Normal);
	bind_attribute("Color", Color);
	bind_attribute("TexCoord", TexCoord);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//Check that all active attributes were bound:
	GLint active = 0;
	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &active);
	assert(active >= 0 && "Doesn't makes sense to have negative active attributes.");
	for (GLuint i = 0; i < GLuint(active); ++i) {
		GLchar name[100];
		GLint size = 0;
		GLenum type = 0;
		glGetActiveAttrib(program, i, 100, NULL, &size, &type, name);
		name[99] = '\0';
		GLint location = glGetAttribLocation(program, name);
		if (!bound.count(GLuint(location))) {
			throw std::runtime_error("ERROR: active attribute '" + std::string(name) + "' in program is not bound.");
		}
	}

	return vao;
}
