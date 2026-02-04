//! Black Hole Simulator - Rust Core
//!
//! Ponte entre o C e o Rust.
//! Aqui definimos a interface segura (ou não) para o código C.

#![no_std]
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

/// Módulo contendo os bindings brutos gerados automaticamente.
/// NÃO EDITE este módulo manualmente.
pub mod bindings;

// Re-exporta para facilitar o uso
pub use bindings::*;

// Funções utilitárias básicos podem ir aqui
