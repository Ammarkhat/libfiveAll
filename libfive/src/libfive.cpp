/*
libfive: a CAD kernel for modeling with implicit functions
Copyright (C) 2017  Matt Keeter

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
// #include <iostream>
// #include <fstream>

#include "libfive.h"

#include "libfive/tree/opcode.hpp"
#include "libfive/tree/tree.hpp"
#include "libfive/tree/template.hpp"

#include "libfive/render/brep/region.hpp"
// #include "libfive/render/brep/contours.hpp"
#include "libfive/render/brep/mesh.hpp"

#include "libfive/solve/bounds.hpp"

using namespace Kernel;

void libfive_mesh_delete(libfive_mesh* m)
{
    delete [] m->verts;
    delete [] m->tris;
    delete m;
}

int libfive_opcode_enum(const char* op)
{
    auto o = Opcode::fromScmString(op);
    return (o == Opcode::INVALID || o == Opcode::LAST_OP) ? -1 : o;
}

int libfive_opcode_args(int op)
{
    return (op >= 0 && op < Opcode::LAST_OP)
        ? Opcode::args(Opcode::Opcode(op))
        : -1;
}

////////////////////////////////////////////////////////////////////////////////

libfive_tree libfive_tree_x() { return new Tree(Tree::X()); }
libfive_tree libfive_tree_y() { return new Tree(Tree::Y()); }
libfive_tree libfive_tree_z() { return new Tree(Tree::Z()); }

libfive_tree libfive_tree_const(float f) { return new Tree(f); }
libfive_tree libfive_tree_var() { return new Tree(Tree::var()); }

bool libfive_tree_is_var(libfive_tree t)
{
    return (*t)->op == Opcode::VAR;
}

float libfive_tree_get_const(libfive_tree t, bool* success)
{
    if ((*t)->op == Opcode::CONST)
    {
        if (success) { *success = true; }
        return (*t)->value;
    }
    if (success) { *success = false; }
    return 0;
}

libfive_tree libfive_tree_nonary(int op)
{
    return new Tree(Opcode::Opcode(op));
}

libfive_tree libfive_tree_unary(int op, libfive_tree a)
{
    return new Tree(Opcode::Opcode(op), *a);
}
libfive_tree libfive_tree_binary(int op, libfive_tree a, libfive_tree b)
{
    return new Tree(Opcode::Opcode(op), *a, *b);
}

const void* libfive_tree_id(libfive_tree t)
{
    return static_cast<const void*>(t->id());
}

void libfive_tree_delete(libfive_tree ptr)
{
    delete ptr;
}

// bool libfive_tree_save(libfive_tree ptr, const char* filename)
// {
//     auto data = ptr->serialize();
//     std::ofstream out;
//     out.open(filename, std::ios::out|std::ios::binary);
//     if (out.is_open())
//     {
//         out.write((const char*)&data[0], data.size());
//         return true;
//     }
//     else
//     {
//         std::cerr << "libfive_tree_save: could not open file" << std::endl;
//         return false;
//     }
// }

// libfive_tree libfive_tree_load(const char* filename)
// {
//     auto t = Tree::load(filename);
//     if (t.id())
//     {
//         return new Tree(t);
//     }
//     else
//     {
//         std::cerr <<  "libfive_tree_load: could not open file" << std::endl;
//         return nullptr;
//     }
// }

libfive_tree libfive_tree_remap(libfive_tree p, libfive_tree x, libfive_tree y, libfive_tree z)
{
    return new Tree(p->remap(*x, *y, *z));
}

float libfive_tree_eval_f(libfive_tree t, libfive_vec3 p)
{
    PointEvaluator e(std::make_shared<Tape>(*t));
    return e.eval({p.x, p.y, p.z});
}

libfive_interval libfive_tree_eval_r(libfive_tree t, libfive_region3 r)
{
    IntervalEvaluator e(std::make_shared<Tape>(*t));
    auto i = e.eval({r.X.lower, r.Y.lower, r.Z.lower},
                    {r.X.upper, r.Y.upper, r.Z.upper});
    return {i.lower(), i.upper()};
}

libfive_vec3 libfive_tree_eval_d(libfive_tree t, libfive_vec3 p)
{
    DerivEvaluator e(std::make_shared<Tape>(*t));
    auto v = e.deriv({p.x, p.y, p.z});
    return {v.x(), v.y(), v.z()};
}

bool libfive_tree_eq(libfive_tree a, libfive_tree b)
{
    return *a == *b;
}

libfive_region3 libfive_tree_bounds(libfive_tree a)
{
    auto bs = findBounds(*a);
    return {{float(bs.lower.x()), float(bs.upper.x())},
            {float(bs.lower.y()), float(bs.upper.y())},
            {float(bs.lower.z()), float(bs.upper.z())}};
}

// char* libfive_tree_print(libfive_tree t)
// {
//     std::stringstream ss;
//     (*t)->print(ss);
//     const auto str = ss.str();

//     auto out = static_cast<char*>(malloc(str.size() + 1 * sizeof(char)));
//     memcpy(out, str.c_str(), str.size() + 1);
//     return out;
// }

////////////////////////////////////////////////////////////////////////////////

// libfive_contours* libfive_tree_render_slice(libfive_tree tree,
//         libfive_region2 R, float z, float res)
// {
//     Region<2> region({R.X.lower, R.Y.lower}, {R.X.upper, R.Y.upper},
//             Region<2>::Perp(z));
//     auto cs = Contours::render(*tree, region, 1/res);

//     auto out = new libfive_contours;
//     out->count = cs->contours.size();
//     out->cs = new libfive_contour[out->count];

//     size_t i=0;
//     for (auto& c : cs->contours)
//     {
//         out->cs[i].count = c.size();
//         out->cs[i].pts = new libfive_vec2[c.size()];

//         size_t j=0;
//         for (auto& pt : c)
//         {
//             out->cs[i].pts[j++] = {pt.x(), pt.y()};
//         }
//         i++;
//     }

//     return out;
// }

// void libfive_tree_save_slice(libfive_tree tree, libfive_region2 R, float z, float res,
//                         const char* f)
// {
//     Region<2> region({R.X.lower, R.Y.lower}, {R.X.upper, R.Y.upper},
//             Region<2>::Perp(z));
//     auto cs = Contours::render(*tree, region, 1/res);
//     cs->saveSVG(f);
// }

libfive_mesh* libfive_tree_render_mesh(libfive_tree tree, libfive_region3 R, float res)
{
    Region<3> region({R.X.lower, R.Y.lower, R.Z.lower},
                     {R.X.upper, R.Y.upper, R.Z.upper});
    auto ms = Mesh::render(*tree, region, 1/res);
    if (ms.get() == nullptr)
    {
        fprintf(stderr, "libfive_tree_render_mesh: got empty mesh\n");
        return nullptr;
    }

    auto out = new libfive_mesh;
    out->verts = new libfive_vec3[ms->verts.size()];
    out->vert_count = ms->verts.size();
    out->tris = new libfive_tri[ms->branes.size()];
    out->tri_count = ms->branes.size();

    size_t i;

    i=0;
    for (auto& v : ms->verts)
    {
        out->verts[i++] = {v.x(), v.y(), v.z()};
    }

    i=0;
    for (auto& t : ms->branes)
    {
        out->tris[i++] = {(uint32_t)t.x(), (uint32_t)t.y(), (uint32_t)t.z()};
    }

    return out;
}

// bool libfive_tree_save_mesh(libfive_tree tree, libfive_region3 R, float res, const char* f)
// {
//     Region<3> region({R.X.lower, R.Y.lower, R.Z.lower},
//                      {R.X.upper, R.Y.upper, R.Z.upper});
//     auto ms = Mesh::render(*tree, region, 1/res);
//     return ms->saveSTL(f);
// }

////////////////////////////////////////////////////////////////////////////////

typedef Tree::Id libfive_id;

struct libfive_args {
    char** names;
    libfive_id* ids;
    libfive_tree* bindings;
    uint32_t count;
};

void libfive_args_delete(libfive_args* a)
{
    for (unsigned i=0; i < a->count; ++i)
    {
        delete [] a->names[i];
        libfive_tree_delete(a->bindings[i]);
    }
    delete [] a->names;
    delete [] a->ids;
    delete [] a->bindings;
    delete a;
}

libfive_args* libfive_args_new(uint32_t count)
{
    auto a = new libfive_args;
    a->count = count;
    a->names = new char*[count];
    a->bindings = new libfive_tree[count];
    a->ids = new Tree::Id[count];

    return a;
}

typedef Kernel::Template* libfive_template;

const char* libfive_arg_name(libfive_args* a, uint32_t i)
{
    return (i >= a->count) ? nullptr : a->names[i];
}

void libfive_set_arg_name(libfive_args* a, uint32_t i, const char* name)
{
    if (i < a->count)
    {
        if (a->names[i] != nullptr)
        {
            delete [] a->names[i];
        }
        auto len = strlen(name);
        a->names[i] = new char[len + 1];
        memcpy(a->names[i], name, len);
        a->names[i][len] = 0; // apply null termination
    }
}

void libfive_set_arg_id(libfive_args* a, uint32_t i, libfive_id id)
{
    if (i < a->count)
    {
        a->ids[i] = id;
    }
}

libfive_template libfive_tree_to_template(libfive_tree t)
{
    return new Template(*t);
}

libfive_args* libfive_template_args(libfive_template t)
{
    auto a = libfive_args_new(t->vars.size());
    uint32_t i=0;

    for (auto v : t->vars)
    {
        libfive_set_arg_name(a, i, v.second.c_str());
        libfive_set_arg_id(a, i, v.first);
        i++;
    }

    return a;
}

////////////////////////////////////////////////////////////////////////////////

// These variables are autogenerated and compiled into the library
// by the version.cmake script
extern "C"
{
    extern const char* GIT_TAG;
    extern const char* GIT_REV;
    extern const char* GIT_BRANCH;
}

const char* libfive_git_version(void)
{
    return GIT_TAG;
}

const char* libfive_git_revision(void)
{
    return GIT_REV;
}

const char* libfive_git_branch(void)
{
    return GIT_BRANCH;
}
